// Exercise actual menu with a deliberately slow display, not just button units.
void slowTftOperation();
#define PUPPYBOT_TFT_OPERATION_HOOK slowTftOperation
#define main originalCalibrationTests
#include "test_line.cpp"
#undef main
bool slowDrawing = true;
void slowTftOperation() { if (slowDrawing) fakeMillis += 12; }
int main() {
 try {
  clicks={{100,true},{300,true},{500,true},{1100,true},{1700,true},{1900,false},{2100,false},{2300,true},
          {2500,false},{2700,true},{2900,true},{3100,true},{3700,true},{4300,true},
          {4500,false},{4700,false},{4900,true},{5100,false},{5300,true}};
  fakeMillis=0;fakeBootHook=boot;fakeDigitalReadHook=sw;
  const int commits=EEPROM.commits;
  runMenu();
  require(EEPROM.commits==commits+2,"60ms SW1 clicks missed with slow TFT");
  std::cout<<"PASS actual menu accepts 60ms BOOT/SW1 clicks with simulated 12ms per TFT operation, captures and saves both banks\n";
 } catch(const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n';return 1; }
}
