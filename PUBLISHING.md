# เผยแพร่ URL

Repository เป้าหมาย: https://github.com/Amornthep-ra/Logistic15U-PuppyBot6

## สถานะ

- **0.1.0:** เผยแพร่บน `main` แล้ว (commit `04f744b`) ผลตรวจ URL อยู่ใน `PUBLIC_URL_VERIFICATION.md`
- **0.2.0:** build ZIP, index และ manifest ในเครื่องแล้ว **ยังไม่ได้ commit หรือ push**

## ไฟล์ของ 0.2.0

| ไฟล์ | สร้างด้วย |
| --- | --- |
| `dist/Logistic15U-PuppyBot6-0.2.0.zip` | `python scripts/build_release.py` |
| `dist/logistic15u-rp2040-0.2.0.zip` | `python scripts/build_release.py` |
| `package_logistic15u_index.json`, `dist/manifest.json` | `python scripts/build_release.py` |
| `dist/Logistic15U-PuppyBot6-repository-0.2.0.zip` | `python scripts/build_source_bundle.py` (รันหลัง build_release) |

ตรวจด้วย `python scripts/verify_distribution.py` ก่อนอัปโหลด

Package index ยังเก็บรายการแพลตฟอร์ม 0.1.0 ไว้ต่อจาก 0.2.0 จึง**ห้ามลบ** `dist/*-0.1.0.zip` ออกจาก repository เพราะ URL ใน index ยังชี้ไปที่ไฟล์นั้น

## ขั้นตอนเผยแพร่

1. commit ไฟล์ทั้งหมดใน repository (รวม `dist/`, `package_logistic15u_index.json`, Lib และเอกสาร) ไม่รวม `.arduino-test/` และ `reference/legacy-imu/` ซึ่ง `.gitignore` กันไว้แล้ว
2. push ไปที่ branch `main` ด้วยบัญชีที่มีสิทธิ์เขียน repository
3. ตรวจว่าลิงก์เหล่านี้ดาวน์โหลดได้โดยไม่ต้องล็อกอิน และ SHA-256 ตรงกับ `dist/manifest.json`:
   - [Board Manager URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json)
   - [Library ZIP URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.2.0.zip)
   - [Board ZIP URL](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.2.0.zip)
4. ทดสอบ `scripts/install.ps1 -ConfigFile <config แยก>` กับ sketchbook ทดสอบ แล้วบันทึกผลใน `PUBLIC_URL_VERIFICATION.md`
5. (แนะนำ) สร้าง tag `v0.2.0` และ GitHub Release แนบ ZIP ทั้งสองไฟล์

การแจก library ด้วย ZIP ใช้ได้ทันทีหลังเผยแพร่ แต่ยังไม่ได้ยื่นลงทะเบียน
Arduino Library Manager กลาง การเพิ่ม Board Manager URL ไม่ได้ติดตั้ง library ให้อัตโนมัติ

## ก่อนเผยแพร่สาธารณะ

ไม่มีไฟล์ LICENSE และ `Logistic15U-PuppyBot6/NOTICE.md` ระบุว่าโค้ด API มาจาก PuppyBot6-New ของ KBIDE โดยไม่ได้ให้สิทธิ์ใหม่ ควรยืนยันสิทธิ์แจกจ่ายต่อกับเจ้าของโค้ดต้นฉบับก่อนประกาศ URL
