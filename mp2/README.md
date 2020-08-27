# MP2 - Building a Fault-Tolerant Key-Value Store 

I was provided with an Emulated Network to mimic a distributed system where nodes would join, fail, or messages would be dropped. This allowed us to run multiple copies of nodes within one process running a single-threaded emulation engine. 

I was tasked with implementing certain features of a Fault Tolerant Key-Value Store. Code is located in MP2Node.cpp file. 

- A key-value store supporting **CRUD** operations (Create, Read, Update, Delete)
- **Load-balancing** (via a consistent hashing ring to hash both servers and keys)
- **Fault-tolerance** up to two failures (by replicating each key three times to three successive nodes in the ring, starting from the first node at or to the clockwise of the hashed key)
- **Quorum consistency level** for both reads and writes (at least two replicas)
- **Stabilization** after failure (recreate three replicas after failure)

Full Instructions: [mp2_specifications.pdf](MP2_specification-document.pdf)