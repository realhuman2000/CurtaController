// include the library code:
#include <LiquidCrystal.h>
#include <SimpleKeypad.h>

// define your keypad layout and pins
const byte nb_rows = 4;               // four rows
const byte nb_cols = 4;               // four columns
char key_chars[nb_rows][nb_cols] = {  // The symbols of the keys
  { '1', '2', '3', '+' },
  { '4', '5', '6', '-' },
  { '7', '8', '9', '*' },
  { 'C', '0', '=', '/' }
};
byte rowPins[nb_rows] = { A7, A6, A5, A4 };  // The pins where the rows are connected
byte colPins[nb_cols] = { A3, A2, A1, A0 };  // The pins where the columns are connected

SimpleKeypad kp1((char *)key_chars, rowPins, colPins, nb_rows, nb_cols);  // New keypad called kp1


// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// define the opperational status states and lights
const int green_light = 5, red_light = 4;
enum STATUS {
  ERROR,
  SUCCESS,
  BUSY,
  OVERFLOW,
  PARTY
};

STATUS status = BUSY;

// Display character buffers
String line_1;
String line_2;
String full_display_buffer;

// Define the length of a single line on our LCD
const int max_line_size = 16;

// set up bounds for the num of digits the curta calc can accept
const int line_1_size = 8;
const int line_2_size = 8;
// define how many digits you can actually show:
const int maxDisplayDigits = 11;

int line_1_count = 0;
int line_2_count = 0;

// define the number of blinks / flashes the display will do after
// all inputs have been completed
const int input_complete_blinks = 3;

void setup() {
  Serial.begin(9600);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // WARNING: pins 0 & 1 are the hardware Serial TX/RX on an AVR!
  // If you depend on Serial, consider moving these to other digital pins.
  pinMode(red_light, OUTPUT);
  pinMode(green_light, OUTPUT);

  // start with both off
  digitalWrite(red_light, HIGH);
  digitalWrite(green_light, HIGH);
  // Print a message to the LCD.
  lcd.print("Curta Calc 2025");
  delay(2222);
  lcd.clear();

  refreshDisplay();
  setStatusLights(SUCCESS);
  Serial.println("Setup Complete");
}

void loop() {
  char key = kp1.getKey();  // The getKey function scans the keypad every 10 ms and returns a key only one time, when you start pressing it
  if (key) {                // If getKey returned any key
    Serial.println(key);    // it is printed on the serial monitor
    processKey(key);
  }
}

///////////////// END LOOP /////////////////

// Centralize all key handling in processKey()
String operand1 = "";
String operand2 = "";
char oper = '\0';
bool editingOp1 = true;

void processKey(char k) {
  if (k >= '0' && k <= '9') {
    // append digit to whichever operand we’re editing
    if (editingOp1) {
      if (operand1.length() < line_1_size) operand1 += k;
    } else {
      if (operand2.length() < line_2_size) operand2 += k;
    }
    refreshDisplay();
  } else if (k == '/') {
    // toggle sign on operand1 (line 1)
    if (operand1.length() > 0) {
      if (operand1.charAt(0) == '-') {
        operand1 = operand1.substring(1);
      } else {
        operand1 = "-" + operand1;
      }
      refreshDisplay();
    }

  } else if (k == '+' || k == '-' || k == '*') {
    int64_t line1 = parseInt64(operand1);

    bool isPartyTime = ([&] {
      if (line1 == 69) return true;
      if (line1 == 8008135) return true;
      if (line1 == 420) return true;
      return false;
    })();

    if (isPartyTime) {
      setStatusLights(PARTY);
    }

    // switch to second operand
    if (operand1.length() > 0) {
      oper = k;
      editingOp1 = false;
    }
  } else if (k == '=') {
    setStatusLights(BUSY);
    // perform calculation
    if (operand1.length() && operand2.length() && oper) {
      int64_t a = parseInt64(operand1);
      int64_t b = parseInt64(operand2);
      int64_t result = 0;
      switch (oper) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
      }

      // logging the calculations
      Serial.print(toString64(a));
      Serial.print(oper);
      Serial.print(toString64(b));
      Serial.print('=');
      Serial.println(toString64(result));

      // check against your 11‑digit display limit
      // (ignore leading ‘–’ when counting)
      bool tooBig = ([&] {
        String s = toString64(result);
        int count = (s.charAt(0) == '-') ? s.length() - 1 : s.length();
        return count > maxDisplayDigits;
      })();

      bool isPartyTime = ([&] {
        if (result == 69) return true;
        if (result == 8008135) return true;
        if (result == 420) return true;
        return false;
      })();

      if (tooBig) {
        showOverflowWarning(result);
        return;
      }

      // simulate axis setup work
      delay(3000);

      if (isPartyTime) {
        setStatusLights(PARTY);
      }

      // otherwise commit:
      operand1 = toString64(result);
      setStatusLights(SUCCESS);
    }
    // reset for next
    operand2 = "";
    oper = '\0';
    editingOp1 = true;
    refreshDisplay();
  } else if (k == 'C') {
    // clear all
    operand1 = "";
    operand2 = "";
    oper = '\0';
    editingOp1 = true;
  }
  refreshDisplay();
}


// Display helper
void refreshDisplay() {
  // build line 1: operand1, padded with ‘_’
  String l1 = operand1;
  while (l1.length() < line_1_size) l1 = l1 + "_";
  while (l1.length() < max_line_size) l1 += ' ';

  // build line 2: operator and operand2
  String l2 = "";  // indent so the ‘=’ lines up
  if (oper) l2 += oper;
  else l2 += ' ';
  l2 += " ";
  l2 += operand2;
  while (l2.length() < line_2_size + 4) l2 += ' ';
  l2 += " -> ";

  // push to the LCD
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void showOverflowWarning(int64_t value) {
  // 1) clear and show warning
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("** OVERFLOW **");
  lcd.setCursor(0, 1);
  lcd.print(toString64(value));
  setStatusLights(OVERFLOW);
  delay(2000);

  //wait for keypress
  bool ack = false;
  while (!ack) {
    ack = kp1.getKey();
  }

  // 2) reset state maintain previous result
  //operand1 = "";
  operand2 = "";
  oper = '\0';
  editingOp1 = true;

  // 3) redraw empty display
  refreshDisplay();
}

int64_t parseInt64(const String &s) {
  int64_t v = 0;
  bool neg = false;
  int start = 0;

  if (s.charAt(0) == '-') {
    neg = true;
    start = 1;
  }
  for (int i = start; i < s.length(); ++i) {
    char c = s.charAt(i);
    if (c >= '0' && c <= '9') {
      v = v * 10 + (c - '0');
    }
  }
  return neg ? -v : v;
}

String toString64(int64_t v) {
  if (v == 0) return "0";
  bool neg = (v < 0);

  // reinterpret the bit‑pattern of v as unsigned and two’s‑complement it
  uint64_t u;
  if (neg) {
    // u = (~v) + 1  correctly computes absolute(v) even when v == INT64_MIN
    u = (~uint64_t(v)) + 1;
  } else {
    u = uint64_t(v);
  }

  char buf[22];
  int i = 0;
  while (u) {
    buf[i++] = '0' + (u % 10);
    u /= 10;
  }
  if (neg) buf[i++] = '-';
  // reverse
  for (int j = 0; j < i / 2; ++j) {
    char t = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = t;
  }
  buf[i] = '\0';
  return String(buf);
}

void setStatusLights(STATUS s) {
  // turn everything off up‑front
  digitalWrite(red_light, LOW);
  digitalWrite(green_light, LOW);

  switch (s) {
    case ERROR:
      Serial.println("Status: Error");
      // pulse red 3x then solid red
      for (int i = 0; i < 3; ++i) {
        digitalWrite(red_light, HIGH);
        delay(500);
        digitalWrite(red_light, LOW);
        delay(500);
      }
      digitalWrite(red_light, HIGH);
      break;

    case SUCCESS:
      Serial.println("Status: Success");
      // pulse green 3x then solid green
      for (int i = 0; i < 3; ++i) {
        digitalWrite(green_light, HIGH);
        delay(200);
        digitalWrite(green_light, LOW);
        delay(200);
      }
      digitalWrite(green_light, HIGH);
      break;

    case BUSY:
      Serial.println("Status: Busy");
      // solid red
      digitalWrite(red_light, HIGH);
      break;

    case OVERFLOW:
      Serial.println("Status: Overflow");
      // pulse red 3× (slower)
      for (int i = 0; i < 3; ++i) {
        digitalWrite(red_light, HIGH);
        delay(500);
        digitalWrite(red_light, LOW);
        delay(500);
      }
      break;

    case PARTY:
      Serial.println("Status: Party");
      // rapid red/green chase
      for (int i = 0; i < 4; ++i) {
        digitalWrite(green_light, HIGH);
        delay(100);
        digitalWrite(red_light, LOW);
        delay(100);
        digitalWrite(green_light, LOW);
        delay(100);
        digitalWrite(red_light, HIGH);
        delay(100);
      }
      digitalWrite(red_light, LOW);
      break;
  }
}
