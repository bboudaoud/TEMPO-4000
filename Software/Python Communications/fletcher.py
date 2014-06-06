"""Routines and classes for computing Fletcher's checksum"""

class fletcher16:
    def __init__(self, byte_data=b''):
        self._sum1 = 0xff
        self._sum2 = 0xff
        self.update(byte_data)
    
    def update(self, byte_data):
        """
        Compute a Fletcher-16 Checksum.
        
        This code mimics the optimized implementation as listed on
        Wikipedia [last checked 6 July 2012]:
        http://en.wikipedia.org/wiki/Fletcher's_checksum#Optimizations
        """
        sum1 = self._sum1
        sum2 = self._sum2
        
        length = len(byte_data)
        byte_iter = iter(byte_data)
        
        while length > 0:
            tlen = min(20, length)
            length -= tlen
            
            while tlen > 0:
                sum1 += next(byte_iter)
                sum2 += sum1
                tlen -= 1
            
            # This performs a partial modulo-255
            # Specifically, it computes x % 256, then adds back x // 256,
            #    which is equivalent to the number of 1s subsumed by using
            #    a 256 divisor rather than 255, but the result can be as
            #    large as 0x1fe, which is > 255
            sum1 = (sum1 & 0xff) + (sum1 >> 8)
            sum2 = (sum2 & 0xff) + (sum2 >> 8)
        
        # Reduce from the possibly-as-large-as-0x1fe to truly modulo-255
        self._sum1 = (sum1 & 0xff) + (sum1 >> 8)
        self._sum2 = (sum2 & 0xff) + (sum2 >> 8)
    
    def digest(self):
        return (self._sum2 << 8) | self._sum1
    
    def copy(self):
        f = fletcher16()
        f._sum1 = self._sum1
        f._sum2 = self._sum2
        return f


class fletcher32:
    def __init__(self, byte_data=b''):
        self._sum1 = 0xffff
        self._sum2 = 0xffff
        self.update(byte_data)
    
    def update(self, byte_data):
        """
        Compute a Fletcher-32 Checksum.
        
        This code mimics the optimized implementation as listed on
        Wikipedia [last checked 6 July 2012]:
        http://en.wikipedia.org/wiki/Fletcher's_checksum#Optimizations
        """
        sum1 = self._sum1
        sum2 = self._sum2
        
        length = len(byte_data)
        byte_iter = iter(byte_data)
        
        while length > 0:
            tlen = min(359, length)
            length -= tlen
            
            while tlen > 0:
                sum1 += next(byte_iter)
                sum2 += sum1
                tlen -= 1
            
            # See analagous comment in fletcher16.update()
            sum1 = (sum1 & 0xffff) + (sum1 >> 16)
            sum2 = (sum2 & 0xffff) + (sum2 >> 16)
        
        # Final reduction step
        self._sum1 = (sum1 & 0xffff) + (sum1 >> 16)
        self._sum2 = (sum2 & 0xffff) + (sum2 >> 16)
    
    def digest(self):
        return (self._sum2 << 16) | self._sum1
    
    def copy(self):
        f = fletcher32()
        f._sum1 = self._sum1
        f._sum2 = self._sum2
        return f


if __name__ == "__main__":
    f = fletcher16()
    f.update(b'helloworld')
    print('With fletcher16...')
    print('helloworld: {0}'.format(f.digest()))
    print('helloworld: {0}'.format(fletcher16(b'helloworld').digest()))
    
    f = fletcher32()
    f.update(b'helloworld')
    print('With fletcher32...')
    print('helloworld: {0}'.format(f.digest()))
    print('helloworld: {0}'.format(fletcher32(b'helloworld').digest()))
