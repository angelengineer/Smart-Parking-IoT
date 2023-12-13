
#include <LiquidCrystal.h>


//LiquidCrystal(rs, rw, enable, d4, d5, d6, d7)


LiquidCrystal lcd(12,11,10,5,4,3,2);
const int PIN_LIBRE = 13;

void setup()
{
  pinMode(PIN_LIBRE, INPUT);


  Serial.begin(115200);
  lcd.begin(16,2);
  lcd.clear();
  lcd.home();
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(0,0);
  lcd.print("PARKING");

  
  

}
void loop() {

  lcd.setCursor(0,1);
  int Libre = digitalRead(PIN_LIBRE);


  if (Libre == HIGH) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("PARKING");
    Serial.println("LIBRE");
    lcd.setCursor(0,1);
    lcd.print("LIBRE");

  } else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("PARKING");
    Serial.println("COMPLETO");
    lcd.setCursor(0,1);
    lcd.print("COMPLETO");
  }



  delay(1000);  

}
