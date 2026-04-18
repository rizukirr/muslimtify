#include "ccompose.h"

int main(void) {
  CC_SetWindow(960, 640, "Muslimtify");
  CC_Init();

  while (CC_Running()) {
    CC_Begin();
    CC_End();
  }

  CC_Shutdown();
  return 0;
}
