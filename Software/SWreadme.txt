The current extent of software-backend support for the TEMPO 4 platform is that of a communicator interface designed to socket into either existing data collection back-ends or the previous created BodyDATA software backend for the TEMPO 3.2 system. The contents of this portion of the repository are organized as follows.

-----------------------------------------------------------------------------
C Code
-----------------------------------------------------------------------------
Contains a simple, first-pass C interface written and natively compiled for Windows (using Cygwin and GCC) to produce binary output files for tranmission to the node using a program such as RealTerm: http://realterm.sourceforge.net/

This C interface was not rigorously tested, and is not recommended for use outside of minimal testing scenarios as it requires intimate knowledge of PC and node side endianess, checksum computation, and byte-parsing.

-----------------------------------------------------------------------------
Python
-----------------------------------------------------------------------------
The provided Python libraries are produced for Python 3.3 atop an existing FTDI library ported from Python 2 for use with the previous TEMPO platform. This FTDI library has been verified working with baud rate up to and beyond 1MHz.

The TEMPO 4 communicator class is tested working from both the command line and simple GUI interfaces designed by an undergraduate student as part of this work. Data session offload coordination is not addressed by the existing code, but time stamp exchange along with a number of platform management commands are.

-----------------------------------------------------------------------------
Areas for Improvement
-----------------------------------------------------------------------------
Software offers up the greatest capacity for improvement of all the work produced as part of the TEMPO 4 platform. As the author is admittedly not a software architect, much of this code could benefit from review and revision with improvements made to both low and high level libraries. Some of these improvements are included below.

1. Addition of commands. Commands to provide additional functionality can be produced at the convenience of the designer and added to both the affiliated software and firmware support code. All commands use simple template-style API to allow for rapid extension and support.

2. GUI oriented offload interface. The most important piece of software integration to be completed for the TEMPO 4 platform is that of a simple, easy-to-interface GUI offload utility for non-technical collaborators involved in remote deployments. Ideally this GUI would constantly run a backgound daemon looking for connected nodes (validated through handshaking) then, once it detects a new node has entered the system, open and either automatically or on-demand allow users to offload the collected data. This automatic offload will be particularly useful in scenarios where the node will be charged, but possibly not explicitly interacted with by the user for long periods of time.

3. Block-oriented offload. The previous TEMPO 3.2 system sped up offload by using a streamed data command that allowed the PC to request a range of card sectors all at once, this mean less coordination and no need for sector-by-sector requests during offload times, speeding up the offload process significantly. In the future it is recommended that a similar command be implemented.

4. Forensic card tools. In addition to block-oriented offload it is recommended that a forensic tool for investigation of TEMPO 4 card structures is produced, possibly oriented around full MMC offload and the 'dd' command in UNIX. This would allow for rapid dumping of the card into a PC-side file, which could then be parsed at far greater speed to detect errors or recover lost file system information. TEMPO 3.2 could also benefit from the creation of such a tool with flexible, possibly python-based, interpretors to parse out file system info on the backend.

5. Node issue tracking and bug reporting. Last but not least, it is recommended that the TEMPO 4 platform have an in-the-field error reporting toolkit developed to make use of cloud/network resources to determine if a node is failing/falling out of calibration and suggest returning the device to the technical collaborator for maintanance. Though this may be a vital consideration for some broadly deployed use-scenarios, it is likely not a critical improvement for technical collaborators looking to deploy the nodes themselves directly.