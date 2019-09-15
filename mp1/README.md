# MP1 - Implementing a Membership Protocol 

We were provided with an Emulated Network to mimic a distributed system where nodes would join, fail, or messages would be dropped. This allowed us to run multiple copies of nodes within one process running a single-threaded emulation engine. We were tasked with implementing any of the membership protocols that were discussed in the course. The three we had to choose from were **All-to-All Heartbeating**, **Gossip-style Heartbeating**, or **SWIM-style membership**. I choose **Gossip-style Heartbeating** although I may extend this project further to also implement SWIM-style membership. 

Full Instructions: [mp1_specifications.pdf](mp1_specifications.pdf)

More Info on Gossip Style Failure Dectection: [GossipFD Paper](https://www.cs.cornell.edu/home/rvr/papers/GossipFD.pdf)

More Info on SWIM Style Membership Protocol: [SWIM Membership Protocol Paper](https://www.cs.cornell.edu/projects/Quicksilver/public_pdfs/SWIM.pdf)

**Fun Fact**: The [Professor](http://indy.cs.illinois.edu/) of this course is one of the creators of the SWIM protocol. So I probably should implement that protocol eventually(lol). 