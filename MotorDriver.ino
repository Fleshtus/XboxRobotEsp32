#include <Bluepad32.h>

// SparkFun Motor Controller
const int PWM1 = 13;
const int PWM2 = 12;
const int Right1 = 14;
const int Right2 = 27;
const int Left1 = 26;
const int Left2 = 25;
const int STBY = 33;


// Bluepad32
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  bool foundController = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
      myControllers[i] = nullptr;
      foundController = true;
      break;
    }
  }

  if (!foundController) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}

void dumpGamepad(ControllerPtr ctl) {
  /*Serial.printf(
        //"idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        //"misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        "axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d\n",
        //ctl->index(),        // Controller Index
        //ctl->dpad(),         // D-pad
        //ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): backwards button
        ctl->throttle()     // (0 - 1023): throttle (AKA gas) button
        //ctl->miscButtons(),  // bitmask of pressed "misc" buttons
        //ctl->gyroX(),        // Gyro X
        //ctl->gyroY(),        // Gyro Y
        //ctl->gyroZ(),        // Gyro Z
        //ctl->accelX(),       // Accelerometer X
        //ctl->accelY(),       // Accelerometer Y
        //ctl->accelZ()        // Accelerometer Z
    );*/
}

int spdTbl[] = {0, 100, 200, 240};
int spdNum = 1;
const double offset = 1.15;
bool turning = false;

void fwd() {
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
}

void bwd() {
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, HIGH);
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, HIGH);
}

void trnR() {
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, HIGH);
}

void trnL() {
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, HIGH);
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
}

void processGamepad(ControllerPtr ctl) {
  // There are different ways to query whether a button is pressed.
  // By query each button individually:
  //  a(), b(), x(), y(), l1(), etc...

  // Change speed
  if (ctl->l1()) {
    if (spdNum - 1 < 0) {
      spdNum = 0;
    }
    else {
      spdNum--;
    }
  }

  if (ctl->r1()) {
    if (spdNum >= 3) {
      spdNum = 3;
    }
    else {
      spdNum++;
    }
    Serial.println(spdNum);
  }
  
  digitalWrite(STBY, HIGH);

  if (ctl->axisX() < -100) {
    turning = true;
    analogWrite(PWM2, spdTbl[spdNum]);
    trnL();

    if (ctl->throttle() > 0) {
      analogWrite(PWM1, 0);
      fwd();
    }
    else if (ctl->brake() > 0) {
      analogWrite(PWM1, 0);
      bwd();
    }
    else {
      analogWrite(PWM1, spdTbl[spdNum]);
    }
  }
  else if (ctl->axisX() > 100){
    turning = true;
    analogWrite(PWM1, spdTbl[spdNum] / offset);
    trnR();

    if (ctl->throttle() > 0) {
      analogWrite(PWM2, 0);
      fwd();
    }
    else if (ctl->brake() > 0) {
      analogWrite(PWM2, 0);
      bwd();
    }
    else {
      analogWrite(PWM2, spdTbl[spdNum]);
    }
  }
  else {
    turning = false;
  }

  // Driving
  if (ctl->throttle() > 0) {  // Forward
    if (turning == false) {
      Serial.println(spdTbl[spdNum] / offset);
      fwd(); // Rotate all wheels forward
      analogWrite(PWM1, spdTbl[spdNum] / offset);
      analogWrite(PWM2, spdTbl[spdNum]);
    }
  }
  else if (ctl->brake() > 0) {  // Backward
    if (turning == false) {
      bwd(); // Rotate all wheels forward
      analogWrite(PWM1, spdTbl[spdNum] / offset);
      analogWrite(PWM2, spdTbl[spdNum]);
    }
  }
  else if (turning == false) {
    analogWrite(PWM1, 0);
    analogWrite(PWM2, 0);
  }

  // Another way to query controller data is by getting the buttons() function.
  // See how the different "dump*" functions dump the Controller info.
  dumpGamepad(ctl);
}

void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        processGamepad(myController);
      } else {
        Serial.println("Unsupported controller");
      }
    }
  }
}

// Arduino setup function. Runs in CPU 1
void setup() {
  // Set driving pins to output
  pinMode(Right1, OUTPUT);
  pinMode(Right2, OUTPUT);
  pinMode(Left1, OUTPUT);
  pinMode(Left2, OUTPUT);

  // Set TB6612 pins to output
  pinMode(STBY, OUTPUT);
  pinMode(PWM1, OUTPUT);
  pinMode(PWM2, OUTPUT);

  Serial.begin(115200);
  //Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  //Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But it might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  // Enables mouse / touchpad support for gamepads that support them.
  // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
  // - First one: the gamepad
  // - Second one, which is a "virtual device", is a mouse.
  // By default, it is disabled.
  BP32.enableVirtualDevice(false);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  bool dataUpdated = BP32.update();
  if (dataUpdated)
    processControllers();

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise, the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  //     vTaskDelay(1);
  delay(150);
}
