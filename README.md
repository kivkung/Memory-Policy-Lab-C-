# Page Replacement Simulator (C)

มินิโปรเจกต์วิชา Operating Systems สำหรับศึกษา **Page Replacement** ซึ่งเป็นส่วนหนึ่งของ Virtual Memory เขียนด้วย C11 ใช้ standard library ไม่มีไลบรารีเสริม หน้าจอเป็นเมนูภาษาอังกฤษใน Terminal เพื่อลดปัญหา encoding

โปรแกรมจำลอง reference string ที่กำหนดไว้ ไม่ได้จัดการ RAM หรือ page table ของระบบปฏิบัติการจริง และไม่ได้จำลอง virtual address translation, TLB, dirty pages หรือเวลาอ่านเขียนดิสก์

## เริ่มใช้งานบน Windows

ต้องมี GCC อยู่ใน PATH ตรวจสอบด้วย `gcc --version` (โปรเจกต์นี้ทดสอบด้วย MinGW GCC 6.3.0)

เปิด PowerShell แล้วรัน:

```powershell
cd "C:\Users\kivku\OneDrive\Documents\2T1\SC362001 Operating Systems\project"
.\build.bat
.\build\simulator.exe
```

หรือดับเบิลคลิก `run.bat` เพื่อ build และเปิดโปรแกรมทันที โดยหน้าต่างจะรอเมื่อโปรแกรมจบ ถ้าแก้โค้ด ให้ build ใหม่ก่อนรันเสมอ `run.bat` จะทำให้ทุกครั้ง

โหลดตัวอย่างตั้งแต่เริ่มได้ด้วย:

```powershell
.\build\simulator.exe examples\classic.txt
```

ถ้า path มีช่องว่าง ให้ครอบด้วย `"..."` โปรแกรมที่เปิดจาก Terminal จะอ้างอิง relative path จาก working directory ปัจจุบัน ส่วน `run.bat` จะเริ่มจากโฟลเดอร์โปรเจกต์

สำหรับ Linux/macOS ที่มี GCC หรือ Clang และ Make:

```sh
make
./build/simulator examples/classic.txt
make test
```

Makefile ใช้คำสั่ง shell แบบ Unix บน Windows ให้ใช้ไฟล์ `.bat` ที่เตรียมไว้ ซอร์สเป็น standard C แต่การรันบน Linux/macOS ยังไม่ได้ทดสอบในเครื่องนี้

## ฟีเจอร์และเมนู

| เมนู | การทำงาน |
|---|---|
| 1 | โหลด reference string จากไฟล์ `.txt` |
| 2 | ตั้งจำนวน frame ตั้งแต่ 1 ถึง 10 |
| 3 | เลือก FIFO, LRU หรือ Optimal |
| 4 | รันทีละขั้น กด Enter เพื่อไปต่อ, `a` รันที่เหลือทั้งหมด, `q` หยุด |
| 5 | แสดงทุกขั้นและผลสรุปในครั้งเดียว |
| 6 | เปรียบเทียบทั้งสาม algorithm ด้วย input และจำนวน frame เดียวกัน |
| 7 | แสดง reference string ที่โหลดไว้ |
| 8 | เปรียบเทียบ page faults เมื่อเพิ่ม frame จาก 1 ถึง 10 และระบุ FIFO Belady's anomaly |
| 0 | ออกจากโปรแกรม |

ค่าเริ่มต้นคือ 3 frames และ FIFO แต่ยังไม่มี input ต้องโหลดไฟล์ก่อน แต่ละรอบเริ่มด้วย frame ว่างเสมอ เลือกเมนู 4 หรือ 5 อีกครั้งเพื่อเริ่มใหม่ ไม่ใช่การ resume รอบก่อนหน้า การเลือก algorithm หรือจำนวน frame จะมีผลกับรอบถัดไป

แถวแรกแสดงทันทีเมื่อเริ่มโหมดทีละขั้น จากนั้นจึงรอคำสั่ง หลังแถวสุดท้ายกด Enter เพื่อดูสรุป หากหยุดก่อนจบ จะแสดง `Stopped` และสถิติเฉพาะส่วนที่ประมวลผลแล้ว

## สร้างไฟล์ input ด้วย Notepad

พิมพ์หมายเลข page เป็นจำนวนเต็มไม่ติดลบ คั่นด้วยช่องว่าง, Tab หรือขึ้นบรรทัดใหม่ เช่น:

```text
7 0 1 2 0 3 0 4
2 3 0 3 2
```

เลือก Save As ตั้งชื่อ `my_input.txt` และ Encoding เป็น **UTF-8** (รองรับทั้งมีและไม่มี BOM) จากนั้นเลือกเมนู 1 และพิมพ์ path ของไฟล์ ไม่ใส่จำนวน frame ในไฟล์ เพราะตั้งจากเมนู 2

- หมายเลข page ใช้ได้ตั้งแต่ `0` ถึง `INT_MAX` ของเครื่อง (2,147,483,647 สำหรับ build ที่ทดสอบ)
- รับได้สูงสุด 1,000 references
- ไม่รับ comma, เลขติดลบ, เครื่องหมายบวก, ทศนิยม, ข้อความ หรือ comment ในไฟล์
- ไฟล์ว่าง ข้อมูลผิดรูปแบบ ตัวเลขล้น และไฟล์ UTF-16 จะแสดงข้อผิดพลาด
- โหลดผิดจะเก็บ reference string ชุดเดิมไว้
- ใช้ชื่อไฟล์และโฟลเดอร์ภาษาอังกฤษจะสะดวกที่สุด: การเปิด path ภาษาไทยผ่าน `fopen` บน Windows ขึ้นอยู่กับ code page ของ runtime จึงไม่รับประกัน Unicode path ทุกกรณี

## อ่านผลอย่างไร

```text
Step       Page |    Frame 1      Frame 2      Frame 3   | Result | Evicted -> Incoming
   1          7 |          7*          -           -   | FAULT | -
     Load page 7 into empty frame 1; no eviction.
```

รูปแบบระยะห่างขึ้นอยู่กับจำนวน frame และความกว้าง Terminal หากเลือก frame มากควรขยายหน้าต่างเพื่อไม่ให้ตารางตัดบรรทัด

- `-` คือ frame ว่าง ไม่ใช่ page 0
- `*` คือ frame ที่ถูกโหลดหรือแทนที่ในขั้นนั้น
- **HIT:** page ที่ร้องขออยู่ใน frame แล้ว
- **FAULT:** page ที่ร้องขอยังไม่อยู่ใน frame รวมกรณีที่ยังมี frame ว่าง
- **Replacement:** เกิด fault ขณะ frame เต็ม จึงต้องเอา page เดิมออก
- **Fault rate:** `faults / references ที่ประมวลผลแล้ว × 100`
- ทุกขั้นระบุเหตุผล และแสดง `page เดิม -> page ใหม่` เมื่อมีการแทนที่

ดังนั้น page faults และ replacements ไม่ใช่จำนวนเดียวกันเสมอไป และ `hits + faults = references ที่ประมวลผลแล้ว`

## หลักการของแต่ละ algorithm

### FIFO — First In, First Out

เอา page ที่เข้ามาก่อนสุดออก ใช้ `fifo_next` ชี้ช่องที่จะถูกแทนที่ เมื่อโหลด page จึงเลื่อน pointer เป็น `(slot + 1) % frame_count` การเกิด hit ไม่เปลี่ยนลำดับ FIFO

### LRU — Least Recently Used

เอา page ที่ไม่ได้ใช้งานมานานที่สุดออก ใช้ `last_used[]` เก็บ step ล่าสุดที่เรียกใช้แต่ละ frame อัปเดตทั้งกรณี hit และ fault เมื่อ frame เต็ม เลือกค่าที่น้อยที่สุด

### Optimal

มอง reference string ที่เหลือ แล้วเอา page ที่จะถูกเรียกใช้อีกครั้งไกลที่สุดในอนาคตออก ถ้าไม่มีการเรียกใช้อีกเลยจะเป็นตัวเลือกที่ดีที่สุด หากหลาย page ไม่ถูกใช้อีก จะเลือก frame หมายเลขต่ำสุดเพื่อให้ผลทำซ้ำได้แน่นอน

Optimal ต้องรู้อนาคต จึงใช้เป็นค่ามาตรฐานเปรียบเทียบ ไม่ใช่ algorithm ที่นำไปใช้กับ workload อนาคตที่ยังไม่รู้ได้โดยตรง

เมื่อกำหนด N เป็นจำนวน references และ F เป็นจำนวน frames: FIFO/LRU ใน implementation นี้ใช้เวลา O(NF) ส่วน Optimal ใช้ O(N²F) เพราะค้นหาอนาคตโดยตรง เหมาะกับข้อมูลจำลองขนาดเล็กนี้ ตัว engine ใช้พื้นที่ทำงาน O(F) เพิ่มจาก input O(N) และไม่เก็บประวัติทุกขั้นไว้พร้อมกัน

## ตัวอย่างที่ควรใช้ตอนนำเสนอ

### 1. เปรียบเทียบ algorithm

โหลด `examples/classic.txt` ตั้ง 3 frames และเลือกเมนู 6:

| Algorithm | Hits | Faults | Replacements | Fault rate |
|---|---:|---:|---:|---:|
| FIFO | 3 | 10 | 7 | 76.92% |
| LRU | 4 | 9 | 6 | 69.23% |
| Optimal | 6 | 7 | 4 | 53.85% |

ผลนี้เป็นของข้อมูลชุดนี้ ไม่ได้หมายความว่า LRU จะชนะ FIFO ทุกชุด

### 2. Belady's anomaly

โหลด `examples/belady.txt` และเลือกเมนู 8:

```text
1 2 3 4 1 2 5 1 2 3 4 5
```

FIFO ที่ 3 frames เกิด **9 faults** แต่ที่ 4 frames กลับเกิด **10 faults** จึงใช้แสดงว่าเพิ่ม frame แล้ว fault อาจเพิ่มขึ้นใน FIFO ได้ เมนู 8 ระบุกรณีที่จำนวน faults เพิ่มจากแถวก่อนหน้า (จำนวน frame ที่ติดกัน)

### 3. Page 0 และการเกิด hit

โหลด `examples/repeated.txt` ซึ่งเป็น `0 0 0 0 0` จะได้ 1 fault และ 4 hits ในทุก algorithm เมื่อมีอย่างน้อย 1 frame

## โครงสร้างและลำดับการอ่านโค้ด

```text
project/
  src/
    simulator.h       โครงสร้างข้อมูลและ interface ของ engine
    simulator.c       FIFO, LRU, Optimal และการนับสถิติ
    input.h / input.c อ่านไฟล์และตรวจสอบ input
    display.h         interface การแสดงผล
    display.c         ตาราง ผลสรุป เปรียบเทียบ และโหมดทีละขั้น
    main.c            เมนูและค่าที่ผู้ใช้เลือก
  examples/           input ตัวอย่าง
  tests/
    test_simulator.c  ทดสอบ engine และ parser
  build.bat           compile สำหรับ Windows
  run.bat             compile แล้วรัน
  test.bat            compile และรันทดสอบ
  Makefile            build สำหรับ Unix shell
  README.md
  build/              executable และไฟล์ชั่วคราวที่สร้างจากการ build
```

แนะนำอ่านตามลำดับ:

1. `simulator.h`: เข้าใจ `ReferenceString`, `Step`, `Stats` และ `Algorithm`
2. `main.c`: ดูว่าผู้ใช้เลือกเมนูแล้วเรียกฟังก์ชันใด
3. `simulator.c`: ไล่การค้นหา hit → หา frame ว่าง → เลือกเหยื่อ → อัปเดตสถิติ
4. `display.c`: ดูวิธีนำ snapshot ไปพิมพ์และรับคำสั่งทีละขั้น
5. `input.c`: ดูการอ่านไฟล์ การข้าม UTF-8 BOM และการตรวจเลขล้น
6. `tests/test_simulator.c`: ดูผลที่คาดหวังและวิธีพิสูจน์ความถูกต้อง

### ข้อมูลหลัก

| ชนิด | หน้าที่ |
|---|---|
| `ReferenceString` | array หมายเลข page และจำนวนสมาชิกที่ใช้งานจริง |
| `Step` | snapshot หลังเรียกใช้หนึ่ง page: frame ทุกช่อง, hit/fault, ช่องที่ใช้, page ที่ออก, เหตุผล |
| `Stats` | จำนวน hits, faults และ replacements |
| `StepCallback` | ฟังก์ชันที่ engine เรียกหลังจบแต่ละ step |

`simulate()` ไม่อ่านคีย์บอร์ดและไม่พิมพ์ตาราง แต่ส่ง `Step` ให้ callback ใน `display.c` ส่วนการเปรียบเทียบส่ง callback เป็น `NULL` เพื่อคำนวณเฉพาะสถิติ Callback คืน 0 เพื่อหยุด หรือคืนค่าที่ไม่ใช่ 0 เพื่อไปต่อ

ตัว `Step` อยู่ใน stack ของ engine หากจะนำไปเก็บประวัติในอนาคต ต้อง **คัดลอก struct** ห้ามเก็บ pointer ไปใช้หลัง callback จบ ส่วน `simulate()` คืน 1 เมื่อ configuration ถูกต้อง รวมกรณี callback สั่งหยุด และคืน 0 เมื่อ configuration ผิด

ในโค้ด index ของ step/frame เริ่มที่ 0 แต่หน้าจอเริ่มที่ 1 และใช้ `EMPTY_PAGE = -1` เพราะ page ที่อนุญาตเป็นจำนวนเต็มไม่ติดลบ

## การทดสอบ

```powershell
.\test.bat
```

เมื่อผ่านจะพิมพ์ `All tests passed ...` และ exit code เป็น 0

ชุดทดสอบตรวจผลที่ทราบล่วงหน้า, FIFO hit ไม่เลื่อนคิว, LRU อัปเดตเมื่อ hit, Belady's anomaly, page 0, INT_MAX, 1/10 frames, การหยุดก่อนจบ และความสอดคล้องของ snapshot ทุกขั้น

สำหรับ Optimal ทดสอบ reference string ความยาว 6 จาก page 0–2 ครบทั้ง 729 ชุด ที่ 1–3 frames เทียบกับการค้นหาทุกทางเลือกเพื่อหาจำนวน fault ต่ำสุดอย่างอิสระ พร้อมตรวจว่า LRU/Optimal ไม่เกิด faults เพิ่มเมื่อเพิ่ม frames ในชุดเหล่านี้

Parser ทดสอบ UTF-8 BOM, CRLF, whitespace, UTF-16 ที่ไม่รองรับ, NUL byte, ไฟล์ว่าง, token ผิด, ตัวเลขล้น, ไฟล์ไม่พบ และขีดจำกัด 1,000 references หากทดสอบด้วยตนเองเพิ่มเติม ลองกรอกเมนูผิด, จำนวน frame 0/11, path ที่มีช่องว่าง และโหลดไฟล์ผิดหลังโหลดไฟล์ถูก

การทดสอบใช้ `assert` จึงอย่าเพิ่ม `-DNDEBUG` ตอน compile tests สคริปต์ที่ให้ไม่ได้กำหนด flag นี้

## แนวทางแบ่งงาน 3 คนและต่อยอด

| คน | ส่วนที่ศึกษา/รับผิดชอบ | สิ่งที่ควรอธิบายได้ |
|---|---|---|
| 1 | `simulator.c/.h` | การเลือก page ที่ออกและการเปลี่ยนสถานะ |
| 2 | `main.c`, `display.c/.h` | เมนู callback และการสรุปผล |
| 3 | `input.c/.h`, tests และตัวอย่าง | validation, test cases และผลทดลอง |

ทุกคนควรเข้าใจทั้งสาม algorithm และลองคำนวณด้วยมืออย่างน้อยหนึ่งชุดก่อนนำเสนอ ขอบเขตปัจจุบันเพียงพอสำหรับ mini project; หากจะต่อยอดภายหลังอาจเพิ่ม Clock/Second Chance หรือ export CSV โดยยังใช้ engine และ callback เดิมเป็นฐานได้
