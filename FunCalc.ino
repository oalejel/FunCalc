
//for drawing to TFT screen
#include <stdint.h>
#include <TFTv2.h>
#include <SPI.h>
#include <SeeedTouchScreen.h> 

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) // mega
#define YP A2   // must be an analog pin, use "An" notation!
#define XM A1   // must be an analog pin, use "An" notation!
#define YM 54   // can be a digital pin, this is A0
#define XP 57   // can be a digital pin, this is A3 

#elif defined(__AVR_ATmega32U4__) // leonardo
#define YP A2   // must be an analog pin, use "An" notation!
#define XM A1   // must be an analog pin, use "An" notation!
#define YM 18   // can be a digital pin, this is A0
#define XP 21   // can be a digital pin, this is A3 

#else //168, 328, something else
#define YP A2   // must be an analog pin, use "An" notation!
#define XM A1   // must be an analog pin, use "An" notation!
#define YM 14   // can be a digital pin, this is A0
#define XP 17   // can be a digital pin, this is A3 

#endif

//Measured ADC values for (0,0) and (210-1,320-1)
//TS_MINX corresponds to ADC value when X = 0
//TS_MINY corresponds to ADC value when Y = 0
//TS_MAXX corresponds to ADC value when X = 240 -1
//TS_MAXY corresponds to ADC value when Y = 320 -1

#define TS_MINX 116*2
#define TS_MAXX 890*2
#define TS_MINY 83*2
#define TS_MAXY 913*2

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// The 2.8" TFT Touch shield has 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM);

//divide: 3; multiply: 7; add: 11; subtract: 13; Clear: 12
String titles[] = {
  "7", "8", "9", "/", "4", "5",
  "6", "x", "1", "2", "3", "+",
  "C", "0", "=", "-"};

String output = "";

boolean add = false;
boolean subtract = false;
boolean multiply = false;
boolean divide = false;

boolean equals = false;

//int _result = 0;//might not need to be doubles for now...
double _operand = 0;
double _operator = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);

  TFT_BL_ON;              //turn on background light?
  Tft.TFTinit();         //init tft library
  drawStart();
  delay(1000);
  drawCalculator();
}

void drawStart() {
  Tft.fillScreen();
  
  Tft.drawString("-- Ti 1000 --", 40, 120, 2, WHITE);
  Tft.drawString("Welcome to", 56, 90, 2, WHITE);
  Tft.drawString("Texas Instruments, 1929-2064", 30, 310, 1, WHITE);
  
  delay(300);
  Tft.drawCircle(100, 200, 30, WHITE);
  delay(300);
  Tft.drawCircle(120, 200, 30, WHITE);
  delay(300);
  Tft.drawCircle(140, 200, 30, WHITE);
}

void loop() {
  //if did no begin, wait for touvch
  if (ts.isTouching()) {
    //react to touches
    Point p = ts.getPoint();
    if (p.z > __PRESURE) {
      p.x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, 320);

      if (p.y > 80) {//chack that it is below the result section
        int button = 0;
        //button height is 60, result screen is 80
        //should go from 80,80 ; 160,80 ; 

        for (int yMult = 1; yMult < 5; yMult++) {
          int ymax = (60 * yMult) + 80;

          for (int xMult = 1; xMult < 5; xMult++) {
            int xmax = (xMult * 60);

            Serial.println(xmax);
            Serial.println(ymax);

            if ((p.x <= xmax) && (p.y <= ymax)) {
              //button at this index is done
              Serial.print("Button is: "); 
              Serial.print(button); 
              Serial.println();
              parseInput(button);
              drawResult();
              delay(400);
              return;
            }
            button++;
          }
        }
      } 
    }
  }

  delay(50);
}

void parseInput(int n) {
  if (n == 12) {
    output = "";
    resetContexts();
    resetOperation();
    return;
  } 
  
    if (equals) {//clear the screen if we saw the result
    output = "";
    equals = false;
  }
  
  /*
  String titles[] = {
  "7", "8", "9", "/", "4", "5",
  "6", "x", "1", "2", "3", "+",
  "C", "0", "=", "-"};
  */
  
  //make sure add, sub etc signs are not alone!!
  //if equals is pressed, show the equal sign, and then parse the result and add to output later
  if (output.length() == 0 && (n == 3 || n== 7 || n == 11 || n == 13 || n == 14 || n == 15)) {
    return;
  }

  //switch for special calculations
  boolean isNumber = true;
  switch (n) {
  case 3:
    isNumber = false;
    resetContexts();
    divide = true;
    removeExtraSymbols();
    break;
  case 7: 
    isNumber = false;
    resetContexts();
    multiply = true;
    removeExtraSymbols();
    break;
  case 11:
    isNumber = false;
    resetContexts();
    add = true;
    removeExtraSymbols();
    break;
  case 15:
    isNumber = false;
    resetContexts();
    subtract = true;
    removeExtraSymbols();
    break;
  case 14: 
    equals = true;
    isNumber = false;
    removeExtraSymbols();//remove times or divide that could be there
    output += "=";
    output += result();
    return;
  }
 
  String appendix = titles[n];
  output += appendix;
}

String getSeparator() {
  if (add) {
    return "+";
  } else if (subtract) {
    return "-";
  } else if (multiply) {
     return "x";   
  } else if (divide) {
    return "/";
  }
  
  return "+";//default
}

void removeExtraSymbols() {
  int finalIndex = output.length() - 1;
  if (finalIndex >= 0) {
    char finChar = output.charAt(finalIndex);
    if (finChar == '+' || finChar == '-' || finChar == '/' || finChar == 'x' || finChar == '=') {
      output.remove(finalIndex, 1);
    }
  }
}

void resetOperation() {
  //  _result = 0;
  _operator = 0;
  _operand = 0;
}

void resetContexts() {
  divide = false;
  multiply = false;
  add = false;
  subtract = false;
  equals = false;
}

double result() {
  int midIndex = output.indexOf(getSeparator());
  Serial.print("this is th symbol index");
  Serial.println(midIndex);
  _operator = output.substring(0, midIndex).toInt();
  _operand = output.substring(midIndex + 1, output.length() - 1).toInt();
  if (add) {
    return _operator + _operand;
  } else if (subtract) {
    return _operator - _operand;
  }  else if (multiply) {
    return _operator * _operand;
  } else if (divide) {
    return _operator / _operand;
  }
}

void drawResult() {
  Tft.fillRectangle(1,0,237, 79, WHITE);

  int length = output.length();
  int fontSize = 5;//default before adjustment
  int y = 20;

  if (length > 18) {
    fontSize = 1;
    y = 38;
  } else if (length > 12) {
    fontSize = 2;
    y = 34;
  } else if (length > 7) {//to save energy, make this else if..
    fontSize = 3;
    y = 25;
  }

  char buffer[50];
  output.toCharArray(buffer, 50);
  Tft.drawString(buffer, 0, y, fontSize, BLACK);
}

void drawCalculator() {
  drawResult();

  int titleIndex = 0;
  for (int y = 80; y < 320; y += 60) {
    for (int x = 0; x < 240; x += 60) {
      Tft.fillRectangle(x,y, 80, 80, BLUE);
      Tft.drawRectangle(x,y, 80, 80, BLACK);
      String title = titles[titleIndex];
      char buf[50];
      title.toCharArray(buf, 50);
      Tft.drawString(buf, x + 25, y + 25, 2, WHITE);
      titleIndex++;
    }
  }
}



