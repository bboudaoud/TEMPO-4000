########################################################################
# This module was inspired by example code in the book Programming
# Python 4e by Mark Lutz, p. 642. That code was obtained from:
# http://examples.oreilly.com/9780596158118/PP4E-Examples-1.2.zip
# and is found in the zip's path:
# PP4E-Examples-1.2/Examples/PP4E/Gui/Tools/threadtools.py
#
# Quoting page xxxvi:
#
# Code Reuse Policies
# We now interrupt this Preface for a word from the legal department.
# This book is here to help you get your job done. In general, you may
# use the code in this book in your programs and documentation. You do
# not need to contact us for permission unless you're reproducing a
# significant portion of the code. For example, writing a program that
# uses several chunks of code from this book does not require
# permission. Selling or distributing a CD-ROM of examples from O'Reilly
# books does require permission. Answering a question by citing this
# book and quoting example code does not require permission.
# Incorporating a significant amount of example code from this book into
# your product's documentation does require permission. We appreciate,
# but do not require, attribution. An attribution usually includes the
# title, author, publisher, and ISBN. For example: "Programming Python,
# Fourth Edition, by Mark Lutz (O'Reilly). Copyright 2011 Mark Lutz,
# 978-0-596-15810-1."
#
# Differences from the example code:
#    Based on threading.Thread, the preferred threading module in Py3
#    The three callback arguments are optional
#    Context is optional (defaults to an empty tuple)
#    The callback queue is an argument to __init__, rather than a global
#    Callbacks are executed directly if no callback queue is given
#    The get_result() and get_exc_info() methods are provided so that
#     the thread can be used without callbacks, e.g.,
#        th = AsyncCall(target=mytarget)
#        th.join()
#        if th.get_exc_info() is None:
#            result = th.get_result()
########################################################################
import threading
import sys
import queue
from functools import wraps

class AsyncCall(threading.Thread):
    """
    Asynchronous (threaded) call to a function or callable
    
    Executes a target function in a separate thread, saving the result
    or exception. Callback hooks are provided for progress updates,
    clean exit, or failure. If a callback queue is provided, then the
    callbacks are enqueued rather than called directly.
    
    If a progress function is provided, it is assumed the target
    callable has a progress keyword argument, which it will call with
    only one argument!
    
    The callback functions take two arguments, as follows:
        on_progress(progress_item, context)
            progress_item is the item passed from the body of target
        on_exit(result, context)
            result is the return value of target
        on_fail(exc_info, context)
            exc_info is a 2-tuple of the exception type and exception
            value raised by the target
    """
    def __init__(self, group=None, target=None, name=None,
                 args=(), kwargs=None, context=(), callback_queue=None,
                 on_exit=None, on_fail=None, on_progress=None, can_kill=False):
        threading.Thread.__init__(self, group, target, name, args, kwargs)
        self.context = context
        self._callback_queue = callback_queue
        self._on_exit = on_exit
        self._on_fail = on_fail
        self._on_progress = on_progress
        self._result = None
        self._exc_info = None
        if can_kill:
            self._kill_evt = threading.Event()
        else:
            self._kill_evt = None
    
    def run(self):
        try:
            if self._target:
                if not self._on_progress:
                    if self._kill_evt is not None:
                        result = self._target(kill_evt=self._kill_evt,
                                              *self._args, **self._kwargs)
                    else:
                        result = self._target(*self._args, **self._kwargs)
                else:
                    def progress(progress_item):
                        self._do_callback(self._on_progress, progress_item)
                    if self._kill_evt is not None:
                        result = self._target(progress=progress,
                                              kill_evt=self._kill_evt,
                                              *self._args, **self._kwargs)
                    else:
                        result = self._target(progress=progress,
                                              *self._args, **self._kwargs)
        except:
            self._exc_info = sys.exc_info()[:2]
            self._do_callback(self._on_fail, self._exc_info)
        else:
            self._result = result
            self._do_callback(self._on_exit, self._result)
        finally:
            # Avoid a refcycle if the thread is running a function with
            # an argument that has a member that points to the thread.
            del self._target, self._args, self._kwargs
    
    def _do_callback(self, func, first_arg):
        args = (first_arg, self.context)
        kwargs = {}     # We don't currently use any kwargs here
        if func:
            if self._callback_queue:
                self._callback_queue.put((func, args, kwargs))
            else:
                func(*args, **kwargs)
    
    def signal_kill(self):
        self._kill_evt.set()
    
    def get_result(self):
        return self._result
    
    def get_exc_info(self):
        return self._exc_info


class Observable:
    """
    Mixin class to support registering observers (callbacks)
    with various events.
    """ 
    def __init__(self, events):
        self._observers = {evt: [] for evt in events}
        self._observer_lock = threading.Lock()
    
    def register_observer(self, event, func):
        with self._observer_lock:
            self._observers[event].append(func)
    
    def unregister_observer(self, event, func):
        with self._observer_lock:
            self._observers[event].remove(func)
    
    def _notify_observers(self, event, *args, **kwargs):
        with self._observer_lock:
            for func in self._observers[event]:
                func(*args, **kwargs)


class CallQueueMachine:
    """
    Mixin class for running a call queue, intended primarily for
    processing callback functions. Methods can be decorated with
    enqueued_method to force them to be run via the queue system.
    """
    def __init__(self):
        self._call_queue = queue.Queue()
    
    def enqueue_dummy_call(self):
        """Useful to unblock process_call_queue(block=True)"""
        self._call_queue.put((lambda: None, (), {}))
    
    def process_call_queue(self, block=True):
        try:
            f, args, kwargs = self._call_queue.get(block=block)
        except queue.Empty:
            pass
        else:
            f(*args, **kwargs)
    
    def _enqueue_func(self, func, *args, **kwargs):
        self._call_queue.put((func, args, kwargs))


def enqueued_method(func):
    """
    Decorator to enqueue rather than directly invoke an instance
    method. Intended for use with a subclass of CallbackMachine.
    """
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        # Note, func has become unbound, must explicitly pass self
        self._enqueue_func(func, self, *args, **kwargs)
    return wrapper


# Begin demonstration code for AsyncCall
#    If you change the args tuple from (4,) to something with a negative
#        value, e.g., (-4,) then it will exercise the failure callback
#    If you don't pass in call_queue (or assign it as None),
#        then the callbacks will be executed directly rather than enqueued
if __name__ == "__main__":
    # TODO: Make better demo code to illustrate everything
    def my_target(value, progress):
        if value < 0:
            raise ValueError
        for i in range(value,value+3):
            progress(i)
        return value + 3
    
    def on_exit(result, context):
        print("Exited with result ({0}) and context ({1})".format(result,
                                                                  context))
    
    def on_fail(exc_info, context):
        print("Failed with exc_info ({0}) and context ({1})".format(exc_info,
                                                                    context))
    
    def on_progress(prog_item, context):
        print("Progress ({0}) with context ({1})".format(prog_item, context))
    
    call_queue = queue.Queue()
    
    af = AsyncCall(target         = my_target,
                   args           = (4,),
                   context        = 'dog',
                   callback_queue = call_queue,
                   on_exit        = on_exit,
                   on_fail        = on_fail,
                   on_progress    = on_progress)
    
    af.start()
    # Callbacks are being executed / enqueued now
    # Let's call join() before doing the remaining stuff, because the
    #    queue checker loop below will exit as soon as the queue is empty
    #    rather than continuing to look for new stuff
    af.join()
    
    # If no callback_queue passed to the AsyncCall, then the following loop
    #  would be omitted because all callbacks would have been executed directly
    empty = False
    while not empty:
        try:
            (func, args) = call_queue.get_nowait()
        except:
            empty = True
        else:
            func(*args)
    
    # You would definitely want to have called join() before doing these calls
    result = af.get_result()
    exc_info = af.get_exc_info()
    print("If I hadn't used callbacks, can still get the stuff from")
    print("the thread after calling join() on it")
    print("So, have result ({0}) and exc_info ({1})".format(result, exc_info))
