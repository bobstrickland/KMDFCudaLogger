GPU Keystroke Logging and Detection on Microsoft Windows
With Kernel Mode Drivers
By Robert Strickland
University of New Orleans
November 2015

Abstract
Computers with high-end video cards are currently vulnerable to malware which can take
advantage of features present on that video card to hide its activities there out of sight of the CPU and
present anti-malware software.

The Direct Memory Access (DMA) functionality that video cards have allows them direct
access to system memory without involving the Central Processing Unit (CPU). While this facility
provides large performance benefits by allowing the video card Graphics Processing Unit (GPU) to do
its massive parallel processing without slowing down (or being slowed down by) the CPU, it also
provides a place for malware to snoop on system memory hidden from current anti-malware software.
This project demonstrates how a particularly valuable memory address, the keystroke buffer,
can be discovered and delivered to a program running on the Graphics Processing Unit (GPU) from a
malicious kernel mode driver in Microsoft Windows without doing memory scans. The program uses
Direct Memory Access (DMA) to view the buffer and records the keystrokes into video card's memory
for a time before encrypting and transmitting them out to another PC.

The second part of this project demonstrates how CPU-bound processes can use the Peripheral
Component Interconnect (PCI) bus to examine video card memory, exposing the activities of any
malware which might be hiding there. Some potential performance issues were discovered and simple
ways of mitigating them were found.

For more details please read the CUDAProjectReport and CUDAProjectPresentation PDF files
