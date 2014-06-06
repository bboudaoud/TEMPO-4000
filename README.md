TEMPO-4000
==========

Hardware, Firmware, and Software for the TEMPO 4000 Platform

This respository contains all documents, files, and information required for production and maintanance of, along with development for the TEMPO 4000 core sampling platform.

The TEMPO 4000 platform is designed to serve the needs of a large range of technical and non-technical collaborators interested in varied levels of customizable 3, 6, and 9 degree-of-freedom inertial motion capture and also to allow rapid development for the wearable context atop an arduino-style standard spacing development header. Those interested in more information about the platform and its origins should contact the INERTIA Team at UVa working under Professor and ECE Department Chair John Lach.

The primary organization of this repository is as follows. There are 4 top-level folders, each of which contains its own more particular readme.txt file discussing both what is included in the directory and future directions which the work may take. This is intended both as a guide for developers interested in working with the system and also researchers interested in extending this work for their own purposes. In the case of published research it is worth noting that the TEMPO 4 platform has not, as of yet, been published on so anyone seeking to use this work as part of a conference or journal publication need seek the explicit permission of the author (bb3jd@virginia.edu) before doing so, otherwise all standard pre-publication rules apply.

The contents of each sub-section of this repository are detailed below:

	Hardware: Board CAM, BoM, and Mentor Graphics PADS 9.3.1 design files
	Firmware: C code for MSP430 and affiliated Doxygen html docs			Software: C and Python interfaces for the PC intended for future work

Please remember that this work is developed explicitly for the purpose of sharing and iterative improvement, as these nodes have not as of yet seen even a small production run (>5 units) it is completely possibly that design flaws in either hardware or firmware have gone ignored. The readme documents at the start of each of these sections serve as a sort of double-purposed errata, so be sure to add/remove from them as problems are discovered/addressed. Most importantly, THIS IS NOT A COMPLETE MOTION CAPTURE SYSTEM, if you are interested in plug-and-play inertial motion capture look elsewhere as this is a system for people interested in DEVELOPING FOR and LEARNING TO DEVELOP FOR the wearable sensor context. There is no shortage of commercial platforms that perform 3, 6, and 9 DoF inertial sensing and if the intent of the deployer is to "set it and forget it" this work is ABSOLUTELY NOT RECOMMENDED FOR USE. I do not take any liability legal or otherwise for the results you obtain using the platform and should consider the use or re-use of any of this code to be taken AT YOUR OWN RISK!

Lastly, while I have provided my contact information and am willing to discuss future extenions of the platform with technical collaborators I have a job and a life, and do not have time to field every question related to device operation. If a targeted question is to be asked about something not provided in this document I am open to discussion. However since the whole point of this platform is to learn and grow as a developer I feel that struggling can be of significant value, and will not sugar coat my answers. I also leave it to those who ask such questions to document the information they find here and in the affiliated readme.txt files, helping to further refine this work for future developers.

I have throughly enjoyed designing this platform and learning all the lesson it had to teach along the way. Though I would be a liar if I said it was without its frustrations, I must say that the interest from my peers and conversations with my advisors and professors were invaluable. I hope this project can serve a similar purpose for future designers, allowing them to grow as designers while also producing neat and useful devices capable of being worn in everyday life.

So with that I hope you find my organization reasonable, code legible, and designs acceptable. 

Happy development,

Ben Boudaoud
(bb3jd@virginia.edu)
