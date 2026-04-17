# ⏱️ Distributed Clock Synchronization System (UDP)

## 👥 Team Members

* **N K Mani Sai Akhil**
  SRN: PES1UG24CS290

* **Narina Jeevan Naga Deep**
  SRN: PES1UG24CS293

* **Nandhakishore PV**
  SRN: PES1UG24CS291

---

## 📌 Overview

This project implements a **Distributed Clock Synchronization System** using **UDP sockets in C**.
It follows a **time request-reply protocol** similar to NTP to synchronize clocks between a client and a server over a network.

---

## 🎯 Objectives

* Implement time synchronization using UDP
* Calculate **clock offset** and **network delay**
* Perform **drift correction**
* Evaluate synchronization **accuracy**

---

## ⚙️ Technologies Used

* C Programming
* UDP Socket Programming
* Linux (Ubuntu / VirtualBox)

---

## 🧩 System Architecture

* **Server**: Handles multiple clients and provides timestamps (T2, T3)
* **Client**: Sends requests, calculates offset, delay, and corrected time

---

## 🔁 Working Principle

### Time Request–Reply Protocol

1. Client sends request at time **T1**
2. Server receives at **T2**
3. Server sends response at **T3**
4. Client receives at **T4**

---

### 📐 Calculations

* **Clock Offset (θ):**
  θ = ((T2 - T1) + (T3 - T4)) / 2

* **Network Delay (δ):**
  δ = (T4 - T1) - (T3 - T2)

---

### 🔧 Drift Correction

Client adjusts its clock using:
**Corrected Time = T4 + Offset**

---

### 📊 Accuracy Evaluation

* Multiple synchronization requests are performed
* Average offset and delay improve accuracy

---

## 🖥️ How to Run

### Step 1: Compile

```bash
gcc server.c -o server -lssl -lcrypto
gcc client.c -o client -lssl -lcrypto
```

### Step 2: Run Server

```bash
./server
```

### Step 3: Run Client

```bash
./client <server_ip>
```

---

## 🌐 Running on Different Systems

* Ensure both systems are connected to the **same WiFi network**
* Use the server’s IP address in the client

Example:

```bash
./client 172.30.93.192
```

---

## 📸 Output Screenshots

### 🖥️ Server Output

![Server Output](screenshots/server_output.png)

### 💻 Client Output

![Client Output](screenshots/client_output.png)

---

## 📁 Project Structure

```
.
├── server.c
├── client.c
├── README.md
├── screenshots/
│   ├── server_output.png
│   └── client_output.png
```

---

## 🚀 Features

* Multi-client support using UDP
* Secure communication (OpenSSL integration)
* Time request-reply synchronization
* Offset and delay computation
* Drift correction
* Accuracy evaluation

---

## 📌 Conclusion

This project demonstrates how distributed systems synchronize clocks using network communication. It highlights the importance of delay and offset calculations in achieving accurate and reliable time synchronization.

---

## 🧠 Key Learning Outcomes

* Understanding of distributed systems
* Hands-on experience with socket programming
* Implementation of synchronization algorithms
* Network communication over LAN

---
