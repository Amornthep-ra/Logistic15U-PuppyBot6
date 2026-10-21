# เผยแพร่ URL

Repository เป้าหมาย: https://github.com/Amornthep-ra/Logistic15U-PuppyBot6

ล็อกอิน GitHub CLI ด้วยบัญชี Amornthep-ra แล้ว และสร้าง Git repository ในเครื่องแล้ว
เตรียม staging สำหรับ 33 ไฟล์แล้ว แต่ยังไม่ได้ commit หรือ push: ระบบอนุมัติของเซสชัน
ปฏิเสธทั้งคำสั่งตั้ง origin และ git commit โดยแจ้งว่าต้องได้รับอนุมัติ
ไฟล์ ZIP และ index ผ่านการติดตั้งในเครื่องแล้ว แต่ URL สาธารณะยังไม่พร้อมใช้

## ไฟล์ที่เตรียมไว้

`dist/Logistic15U-PuppyBot6-repository-0.1.0.zip` เป็นชุดซอร์สสำหรับเผยแพร่
สร้างด้วย `python scripts/build_source_bundle.py` โดยเลือกเฉพาะไฟล์ที่ต้องแจกจ่าย
ไม่มี `.arduino-test`, build cache หรือ `reference/legacy-imu`

แตก ZIP นี้ในโฟลเดอร์ใหม่ แล้วอัปโหลดเนื้อหาไปยัง branch `main` ของ repository
โดยวาง `package_logistic15u_index.json` ไว้ที่ root และเก็บโฟลเดอร์ `dist` ตามเดิม
ใช้ GitHub Desktop หรือ Git ที่ล็อกอินด้วยบัญชีซึ่งมีสิทธิ์เขียน repository นี้

หลังอัปโหลด ตรวจว่าลิงก์เหล่านี้ดาวน์โหลดได้โดยไม่ต้องล็อกอิน:

- [Board Manager URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json)
- [Library ZIP URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.1.0.zip)
- [Board ZIP URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.1.0.zip)

ตรวจ SHA-256 ของ ZIP ที่ดาวน์โหลดเทียบกับ `dist/manifest.json` แล้วทดสอบ
`scripts/install.ps1` กับ sketchbook สำหรับทดสอบก่อนประกาศ URL ให้ผู้ใช้
เมื่อผ่านแล้วจึงอัปเดตข้อความสถานะใน README และ VERIFICATION

การแจก library ด้วย ZIP ใช้ได้ทันทีหลังเผยแพร่ แต่ยังไม่ได้ยื่นลงทะเบียน
Arduino Library Manager กลาง การเพิ่ม Board Manager URL ไม่ได้ติดตั้ง library ให้อัตโนมัติ

ตรวจ URL สาธารณะวันที่ 2026-10-21: package index, Library ZIP และ Board ZIP ตอบ HTTP 404 ทั้งสามรายการ เพราะยังไม่มีไฟล์ใน repository
