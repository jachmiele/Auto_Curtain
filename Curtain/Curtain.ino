//Optymalizacja kodu
#define BLYNK_NO_BUILTIN   // Wylaczenie wbudowanej obslugi pinow GPIO blynk
#define BLYNK_NO_FLOAT     // Wylaczenie operacji zmiennoprzecinkowych

//Port debugowania
//#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

const int in1 = 4;
const int in2 = 12;
const int in3 = 14;
const int in4 = 5;
const long int motor_time = 2;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "Twoj-Token";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Twoje-SSID";
char pass[] = "Twoje-Haslo";

byte i = 0;
int timerId, steps = 20000, stepsCount = 0, light_value = 0;
String modeR;
bool working = false, alarm = false, is_open = false;

byte motor_steps[4] = {
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001,
};

WidgetLED led1(V2);

BlynkTimer timer;

BLYNK_WRITE(V0) //Wirtualny pin V0 - przesylanie komend sterujacych
{
  modeR = param.asStr();
  alarm = true;
}

BLYNK_WRITE(V3) //Wirtualny pin V3 - przesylanie liczby krokow silnika (w tysiacach)
{
  steps = 1000 * param.asInt();
}

void light_measurement()
{
  //funkcja odpowiada za:
  //odczytywanie wartosci natezenia siwatla pobranej z przetwornika ADC,
  //przezlanie jej na wirtualny pin V1 oraz
  //reagowanie na komendy sterujace (modeR)
  
  light_value = analogRead(A0);
  Blynk.virtualWrite(V1, light_value);
  
  //send_debug();

  if(alarm && !working) //jesli przslano komende i urzadenie zrealizowalo juz poprzednia komende
  {
    if(modeR == "day" && !is_open)  //komenda 'dzien' && okno zasloniete
    {
      if(light_value > 500) //kontrola swiatla
      {
        //jesli slonce juz swieci to zwin zaslone
        timerId = timer.setInterval(motor_time, right);
        working = true;
      }
      else
      {
        //jesli slonce nie swieci to przerwij funkcje (czekaj az wzejdzie slonce)
        return;
      }
    }
    if(modeR == "night" && is_open) //komenda 'noc' && okno odsloniete
    {
      timerId = timer.setInterval(motor_time, left);
      working = true;
    }
    if(modeR == "1")  //komenda '1' - zmiana stanu (wywolane recznie z aplikacji)
    {
      if(is_open) //zmien stan zaslony na odwrotny - odslon/zaslon
      {
        timerId = timer.setInterval(motor_time, left);
        working = true;
      }
      else
      {
        timerId = timer.setInterval(motor_time, right);
        working = true;
      }
    }
    alarm = false;  //przeslane polecenie zostalo obsluzone
  }
}

//silnik krokowy zaslana okno
void left()
{
  digitalWrite(in4, bitRead(motor_steps[i], 0));
  digitalWrite(in3, bitRead(motor_steps[i], 1));
  digitalWrite(in2, bitRead(motor_steps[i], 2));
  digitalWrite(in1, bitRead(motor_steps[i], 3));
  i = i > 3 ? 0 : i + 1;
  stepsCount++;
  if(stepsCount >= steps){
    turnOFF();
    is_open = false;
    stepsCount = 0;
    timer.deleteTimer(timerId);
    working = false;
    led1.off();
  }
}

//silnik krokowy odslania okno
void right()
{
  digitalWrite(in1, bitRead(motor_steps[i], 0));
  digitalWrite(in2, bitRead(motor_steps[i], 1));
  digitalWrite(in3, bitRead(motor_steps[i], 2));
  digitalWrite(in4, bitRead(motor_steps[i], 3));
  i = i > 3 ? 0 : i + 1;
  stepsCount++;
  if(stepsCount >= steps){
    turnOFF();
    is_open = true;
    stepsCount = 0;
    timer.deleteTimer(timerId);
    working = false;
    led1.on();
  }
}

//wylacz silnik krokowy - minimalizacja poboru pradu
void turnOFF()
{
   digitalWrite(in4, 0);
   digitalWrite(in3, 0);
   digitalWrite(in2, 0);
   digitalWrite(in1, 0);
}
/*
void send_debug()
{
  Serial.print("Mode: ");
  Serial.print(modeR);
  Serial.print(" Steps: ");
  Serial.print(steps, DEC);
  Serial.print(" Alarm: ");
  Serial.print(alarm, BIN);
  Serial.print("\n");
}
*/
void setup()
{
  //Konfiguracja pinow silnika (sterownik)
  pinMode(in1, OUTPUT);   //pin D0
  pinMode(in2, OUTPUT);   //pin D1
  pinMode(in3, OUTPUT);   //pin D2
  pinMode(in4, OUTPUT);   //pin D3
  
  //Debug console
  //Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(10000L, light_measurement); // Odpytuj co 10sek
}

void loop()
{
  Blynk.run(); //Komunikacja z serwerem Blynk
  timer.run(); //Inicjacja BlynkTimer
}
