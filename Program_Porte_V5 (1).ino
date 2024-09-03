//############################################
//#  Programme Arduino Porte De Poulailler   #
//#       Créer Par Keya Christelle          #
//############################################
//#  Versions:5.0                            #
//############################################

//----Library------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <Wire.h> 
#include <A4988.h>
#include <DS3231.h>
#include <PinButton.h>         //!!!ATTENTION!!! -> changement effectuer dans la librairie (voir ligne 55, 57, 77, 93)
#include <ItemSubMenu.h>
#include <ItemCommand.h>
#include <LcdMenu.h>           //!!!ATTENTION!!! -> changement effectuer dans la librairie (voir ligne 144, 151, 172, 178)
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
//#include <SoftwareSerial.h>


//----Variable-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define ADRESSE_LCD             0x3F
#define MONTER_MOTEUR           1
#define DESCENDRE_MOTEUR        0
#define SPEED                   1000
#define PIN_STEP                3
#define PIN_DIR                 4
#define PIN_ANALOG              A0
#define PIN_BOUTON_DESCENDRE    6
#define PIN_BOUTON_MONTER       7
#define PIN_BOUTON_UP           8
#define PIN_BOUTON_DOWN         9
#define PIN_BOUTON_SELECT       10
#define PIN_ENDSTOP_BAS         11
#define PIN_ENDSTOP_HAUT        12

#define ADDR_HEURE_TIMER_MATIN    0  // 1 octet
#define ADDR_MINUTE_TIMER_MATIN   1  // 1 octet
#define ADDR_SECOND_TIMER_MATIN   2  // 1 octet
#define ADDR_HEURE_TIMER_SOIR     3  // 1 octet
#define ADDR_MINUTE_TIMER_SOIR    4  // 1 octet
#define ADDR_SECOND_TIMER_SOIR    5  // 1 octet
#define ADDR_RANGE_MIN            6  // 2 octets
#define ADDR_RANGE_MAX            8  // 2 octets
#define ADDR_SELECTED_OPTION      10  // 1 octets

bool clearMotor = false;
bool clearHeure = false;
bool clearMenu = false;

bool menuActif = false;
bool sous_Menu = false;

int selection = 0;
int indexMenu = 0;
int selectedOption = 0;
int currentOption = 1;

int heure_now, minute_now, second_now;
int jour, mois, annee;
int heure_TimerMatin, minute_TimerMatin, second_TimerMatin;
int heure_TimerSoir, minute_TimerSoir, second_TimerSoir;
int rangeMin, rangeMax;
int brightnessValue;

/*String receivingAndData;
String jsonString;
String liste[]={"","","","","","","","","","","","","","",""};
*/

/*const char* DaysInEnglish[] = {
  "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};
const char* DaysToFrench[] = {
  "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"
};

int  moisEnNumero(String mois) {
    mois.toLowerCase();
    if (mois == "janvier") return 1;
    else if (mois == "février") return 2;
    else if (mois == "mars") return 3;
    else if (mois == "avril") return 4;
    else if (mois == "mai") return 5;
    else if (mois == "juin") return 6;
    else if (mois == "juillet") return 7;
    else if (mois == "août") return 8;
    else if (mois == "septembre") return 9;
    else if (mois == "octobre") return 10;
    else if (mois == "novembre") return 11;
    else if (mois == "décembre") return 12;
    else return -1;
}

const char* convertDaysToFrench(const char* valueToConvert) {
  for (int i = 0; i < 7; i++) {
    if (strcmp(valueToConvert, DaysInEnglish[i]) == 0) {
      return DaysToFrench[i];
    }
  }
  // Si le jour n'est pas trouvé, retourner une chaîne vide ou un message d'erreur
  return "Jour inconnu";
}*/
int DaysInMonth(int mois, int annee) {
  if (mois == 2) {
    return (isLeapYear(annee)) ? 29 : 28;
  }
  if (mois == 4 || mois == 6 || mois == 9 || mois == 11) {
    return 30;
  }
  return 31;
}
bool isLeapYear(int annee) {
  if (annee % 4 == 0) {
    if (annee % 100 == 0) {
      return (annee % 400 == 0);
    }
    return true;
  }
  return false;
}

//----Menu---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void commandHeure();
void commandDate();
void commandMatin();
void commandSoir();
void commandMin();
void commandMax();

extern MenuItem* settingsTimer[];
extern MenuItem* settingsRange[];


MAIN_MENU(
  ITEM_COMMAND("HEURE", commandHeure),
  ITEM_COMMAND("DATE ", commandDate),
  ITEM_COMMAND("AUTO ", setAuto),
  ITEM_SUBMENU("TIMER", settingsTimer),
  ITEM_SUBMENU("RANGE", settingsRange)
);


SUB_MENU(settingsTimer, mainMenu,
  ITEM_COMMAND("MATIN", commandMatin),
  ITEM_COMMAND("SOIR ", commandSoir)
);

SUB_MENU(settingsRange, mainMenu,
  ITEM_COMMAND("MIN  ", commandMin),
  ITEM_COMMAND("MAX  ", commandMax)
);

//----Définition---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
LiquidCrystal_I2C lcd(ADRESSE_LCD,16,2);
LcdMenu menu(2, 16);
A4988 stepper(PIN_DIR, PIN_STEP);
DS3231  rtc(SDA, SCL);
Time  t;
PinButton boutonDescendre(PIN_BOUTON_DESCENDRE );
//SoftwareSerial Ser(0, 1);
//----Setup--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  //----Console serial & Bluetooth---------------------------------------------------
  Serial.begin(9600);
  //Ser.begin(9600);
  //----LCD--------------------------------------------------------------------------
  lcd.init(); 
  lcd.backlight();
  //----Menu-------------------------------------------------------------------------
  menu.setupLcdWithMenu(0x3F, mainMenu);
  //----Moteur-----------------------------------------------------------------------
  stepper.begin();
  stepper.setSpeed(SPEED);
  //----DS3231-----------------------------------------------------------------------
  rtc.begin();
  //----Pin_Button-------------------------------------------------------------------
  pinMode(PIN_BOUTON_MONTER, INPUT);
  pinMode(PIN_BOUTON_UP, INPUT);
  pinMode(PIN_BOUTON_DOWN, INPUT);
  pinMode(PIN_BOUTON_SELECT, INPUT);
  pinMode(PIN_ENDSTOP_BAS, INPUT);
  pinMode(PIN_ENDSTOP_HAUT, INPUT);
  //---Chargement des valeurs depuis l'EEPROM----------------------------------------
  loadValuesFromEEPROM();
}

//----Loop---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
	
  static bool monterEnCours = false;
  static bool descendreEnCours = false;
  static bool menuActif = false; 
  static bool sousMenuActif = false;
  static bool motorDisplay = false;
  
  //receiverbluetooth();
	while (Serial.available() > 0) {
		String receivingAndData = Serial.readStringUntil("\n");

    if (receivingAndData == "MONTER"){
			monterEnCours = true;
		}
		if (receivingAndData == "DESCENDRE"){
			descendreEnCours = true;
		}
	}
	
  boutonDescendre.update();
  
  if (digitalRead(PIN_BOUTON_SELECT) == HIGH) {
    if (!menuActif) {
      menuActif = true;
      sousMenuActif = false; 
      clearMenu = false;
    }
    delay(150);
  }

  if (menuActif) {
    
    getMenu(); 
    
    if (boutonDescendre.isDoubleClick()) {
      resetToMainMenu();
      saveSetMenu();
      delay(150);
    }
    if (boutonDescendre.isLongClick()) {
      resetToMainMenu();
      menuActif = false;
      delay(150);
    }
    return; 
  }
  //----Moteur-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  if (digitalRead(PIN_BOUTON_MONTER) == HIGH || monterEnCours) {
    motorDisplay = true;
    getMotor(PIN_ENDSTOP_HAUT, monterEnCours, MONTER_MOTEUR, "Ouverture...");
  } else if (boutonDescendre.isSingleClick() || descendreEnCours) {
    motorDisplay = true;
    getMotor(PIN_ENDSTOP_BAS, descendreEnCours, DESCENDRE_MOTEUR, "Fermeture...");
  } else {
    motorDisplay = false;
  }
  //----Mode automatique-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  if (selectedOption == 1){
    //timer
    
    if ((heure_now == heure_TimerMatin) && (minute_now == minute_TimerMatin) && (second_now == second_TimerMatin)){
      getMotor(PIN_ENDSTOP_HAUT, monterEnCours, MONTER_MOTEUR, "Ouverture...");
    }
    
    else if ((heure_now == heure_TimerSoir) && (minute_now == minute_TimerSoir) && (second_now == second_TimerSoir)){
      getMotor(PIN_ENDSTOP_BAS, descendreEnCours, DESCENDRE_MOTEUR, "Fermeture...");
    }
  }else if (selectedOption == 2){
    //range
    brightnessValue = analogRead(A0);
    if (brightnessValue == rangeMin) {
      getMotor(PIN_ENDSTOP_BAS, descendreEnCours, DESCENDRE_MOTEUR, "Fermeture...");
    }
    else if (brightnessValue == rangeMax) {
      getMotor(PIN_ENDSTOP_HAUT, monterEnCours, MONTER_MOTEUR, "Ouverture...");
    }
  }

  if (!motorDisplay) {
    showDateHeure();
  }
  
}

//----Fonction-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void showDateHeure() {
  if (!clearHeure) {
    lcd.clear();
    clearHeure = true;
    clearMenu = false;
    clearMotor = false;
  }
  
  //const char* jourFrancais = convertDaysToFrench(rtc.getDOWStr());
  
  t = rtc.getTime();
  heure_now = t.hour;
  minute_now = t.min;
  second_now = t.sec;
  
  jour = t.date;
  mois = t.mon;
  annee = t.year;

  lcd.setCursor(4, 0);
  lcd.print((heure_now < 10 ? "0" : "") + String(heure_now) + ":");
  lcd.print((minute_now < 10 ? "0" : "") + String(minute_now) + ":");
  lcd.print((second_now < 10 ? "0" : "") + String(second_now));

  //lcd.setCursor(1, 1);
  //lcd.print(jourFrancais);
  lcd.setCursor(3, 1);
  lcd.print((jour < 10 ? "0" : "") + String(jour) + "/");
  lcd.print((mois < 10 ? "0" : "") + String(mois) + "/");
  lcd.print(String(annee));
  
  
}

void getMotor(int pinEndStop, bool &enCours, int direction, const char* message) {
  enCours = true;
  if (digitalRead(pinEndStop) == LOW) {
    stepper.step(direction, 200);

    if (!clearMotor) {
      lcd.clear();
      clearMotor = true;
      clearHeure = false;
      clearMenu = false;
    }

    lcd.setCursor(0,0);
    lcd.print(message);
      
  } else {
    enCours = false;
  }
}

void getMenu(){  
  if (!sous_Menu) {
    if (!clearMenu) {
      lcd.clear();
      clearMenu = true;
      clearHeure = false;
      clearMotor = false;
      
      menu.resetMenu();
    }

    lcd.setCursor(0, 0);
    lcd.print("REGLAGE:");
    controlMenu(); 

    
  } else {
    SetMenu();
  }
}

void controlMenu() {
  if (digitalRead(PIN_BOUTON_UP) == HIGH) {
    menu.up();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_DOWN) == HIGH) {
    menu.down();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_SELECT) == HIGH) {
    menu.enter();
    delay(150);
  }
}

void resetToMainMenu() {
  sous_Menu = false;
  selection =0;
  lcd.clear();
  menu.back();
  menu.show();
  delay(150);
}

void SetMenu() {
  switch (indexMenu) {
    case 1:
      setHeure(commandHeure, heure_now, minute_now, second_now);
      break;
    case 2:
      setDate(commandDate, jour, mois, annee);
      break;
    case 3:
      setHeure(commandMatin, heure_TimerMatin, minute_TimerMatin, second_TimerMatin);
      break;
    case 4:
      setHeure(commandSoir, heure_TimerSoir, minute_TimerSoir, second_TimerSoir);
      break;
    case 5:
      setRange(commandMin, rangeMin);
      break;
    case 6:
      setRange(commandMax, rangeMax);
      break;
  }
}

void setHeure(void (*commandFunction)(), int &heure, int &minute, int &second) {
  if (digitalRead(PIN_BOUTON_UP) == HIGH) {
    adjustTime(selection, heure, minute, second, 1);
    commandFunction();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_DOWN) == HIGH) {
    adjustTime(selection, heure, minute, second, -1);
    commandFunction();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_SELECT) == HIGH) {
    selection = (selection + 1) % 3;
    commandFunction();
    delay(150);
  }
}

void setDate(void (*commandFunction)(), int &jour, int &mois, int &annee) {
  if (digitalRead(PIN_BOUTON_UP) == HIGH) {
    adjustDate(selection, jour, mois, annee, 1);
    commandFunction();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_DOWN) == HIGH) {
    adjustDate(selection, jour, mois, annee, -1);
    commandFunction();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_SELECT) == HIGH) {
    selection = (selection + 1) % 3;
    commandFunction();
    delay(150);
  }
}

void setRange(void (*commandFunction)(), int &range) {
  if (digitalRead(PIN_BOUTON_UP) == HIGH) {
    range = (range + 1) % 1000;
    commandFunction();
    delay(150);
  }

  if (digitalRead(PIN_BOUTON_DOWN) == HIGH) {
    range = (range - 1 + 1000) % 1000;
    commandFunction();
    delay(150);
  }
}

void adjustTime(int selection, int &heure, int &minute, int &second, int increment) {
  if (selection == 0) {
    heure = (heure + increment + 24) % 24;
  } else if (selection == 1) {
    minute = (minute + increment + 60) % 60;
  } else if (selection == 2) {
    second = (second + increment + 60) % 60;
  }
}

void adjustDate(int selection, int &jour, int &mois, int &annee, int increment) {
  if (selection == 0) {
    jour += increment;
    while (jour > DaysInMonth(mois, annee)) {
      jour -= DaysInMonth(mois, annee);
      mois++;
      if (mois > 12) {
        mois = 1;
        annee++;
      }
    }
    while (jour < 1) {
      mois--;
      if (mois < 1) {
        mois = 12;
        annee--;
      }
      jour += DaysInMonth(mois, annee);
    }
  } else if (selection == 1) { // Mois
    mois += increment;
    while (mois > 12) {
      mois -= 12;
      annee++;
    }
    while (mois < 1) {
      mois += 12;
      annee--;
    }

    jour = min(jour, DaysInMonth(mois, annee));
  } else if (selection == 2) { // Année
    annee += increment;
    if (annee < 2024) {
      annee = 2100;
    } else if (annee > 2100) {
      annee = 2024;
    }
  }
}

void commandHeure() {
  showSetMenu("HEURE:", heure_now, minute_now, second_now, 1);
}
void commandDate() {
  showSetMenu("DATE:", jour, mois, annee, 2);
}
void commandMatin() {
  showSetMenu("MATIN:", heure_TimerMatin, minute_TimerMatin, second_TimerMatin, 3);
}
void commandSoir() {
  showSetMenu("SOIR:", heure_TimerSoir, minute_TimerSoir, second_TimerSoir, 4);
}
void commandMin() {
  showSetMenu("MIN:", rangeMin, 0, 0, 5);
}
void commandMax() {
  showSetMenu("MAX:", rangeMax, 0, 0, 6);
}
void showSetMenu(const char* title, int val1, int val2, int val3, int pos) {
  menu.hide();
  lcd.clear();
  sous_Menu = true;
  indexMenu = pos;

  lcd.setCursor(0, 0);
  lcd.print(title);

  lcd.setCursor(4, 1);
  if (indexMenu == 2) { // Date
    lcd.print((val1 < 10 ? "0" : "") + String(val1) + "/");
    lcd.print((val2 < 10 ? "0" : "") + String(val2) + "/");
    lcd.print(String(val3));
  } else if (pos == 5 || pos == 6) { // Range Min, Max
    lcd.print((val1 < 10 ? "00" : (val1 < 100 ? "0" : "")) + String(val1));
  } else { // Heure Now ou Timer Matin, Soir
    lcd.print((val1 < 10 ? "0" : "") + String(val1) + ":");
    lcd.print((val2 < 10 ? "0" : "") + String(val2) + ":");
    lcd.print((val3 < 10 ? "0" : "") + String(val3));
  }
}

void setAuto(){
  while (true){
    if (digitalRead(PIN_BOUTON_UP) == HIGH) {
      currentOption = 1;
      displayOptions();
      delay(150);
    }
    if (digitalRead(PIN_BOUTON_DOWN) == HIGH) {
      currentOption = 2;
      displayOptions();
      delay(150);
    }
    if (digitalRead(PIN_BOUTON_SELECT) == HIGH) {
      
      if (currentOption != selectedOption) {
        selectedOption = currentOption;
      } else {
        selectedOption = 0; 
      }
      displayOptions();
      delay(250);
    }
    
    
    boutonDescendre.update();
    if (boutonDescendre.isDoubleClick()) {
      resetToMainMenu();
      saveSetMenu();
      break;
      delay(150);
    }
    if (boutonDescendre.isLongClick()) {
      resetToMainMenu();
      menuActif = false;
      break;
      delay(150);
    }
  }
}

void displayOptions() {
  menu.hide();
  lcd.clear();
  sous_Menu = true;


  displayOption(" Timer", 0, 1);
  displayOption(" Range", 1, 2);
}
void displayOption(String option, int row, int optionNumber) {
  lcd.setCursor(0, row);

  if (currentOption == optionNumber) {
    lcd.write(0x7E);
  } else {
    lcd.print(" ");
  }
  if (selectedOption == optionNumber) {
    lcd.print(" [x]");
  } else {
    lcd.print(" [ ]");
  }
  lcd.print(option);
  
}

void saveSetMenu() {
  rtc.setTime(heure_now, minute_now, second_now);
  rtc.setDate(jour, mois, annee);

  EEPROM.write(ADDR_HEURE_TIMER_MATIN, heure_TimerMatin);
  EEPROM.write(ADDR_MINUTE_TIMER_MATIN, minute_TimerMatin);
  EEPROM.write(ADDR_SECOND_TIMER_MATIN, second_TimerMatin);
  EEPROM.write(ADDR_HEURE_TIMER_SOIR, heure_TimerSoir);
  EEPROM.write(ADDR_MINUTE_TIMER_SOIR, minute_TimerSoir);
  EEPROM.write(ADDR_SECOND_TIMER_SOIR, second_TimerSoir);

  EEPROM.put(ADDR_RANGE_MIN, rangeMin);
  EEPROM.put(ADDR_RANGE_MAX, rangeMax);
  EEPROM.put(ADDR_SELECTED_OPTION, selectedOption);
  sendbluetooth();
}
void loadValuesFromEEPROM() {

  heure_TimerMatin = EEPROM.read(ADDR_HEURE_TIMER_MATIN);
  minute_TimerMatin = EEPROM.read(ADDR_MINUTE_TIMER_MATIN);
  second_TimerMatin = EEPROM.read(ADDR_SECOND_TIMER_MATIN);
  heure_TimerSoir = EEPROM.read(ADDR_HEURE_TIMER_SOIR);
  minute_TimerSoir = EEPROM.read(ADDR_MINUTE_TIMER_SOIR);
  second_TimerSoir = EEPROM.read(ADDR_SECOND_TIMER_SOIR);
  
  EEPROM.get(ADDR_RANGE_MIN, rangeMin);
  EEPROM.get(ADDR_RANGE_MAX, rangeMax);
  EEPROM.get(ADDR_SELECTED_OPTION, selectedOption);
}

void receiverbluetooth(){
  while (Serial.available() > 0) {
    String receivingAndData = Ser.readStringUntil("\n");

    if (receivingAndData == "BLT_OK"){
      
      sendbluetooth();

    } else if (receivingAndData != "") {
      Serial.println(receivingAndData);
    }
    
    if (receivingAndData.startsWith("Save")) {
      String jsonString = receivingAndData.substring(4);
      jsonString.trim();
      jsonString.replace("{","");
      jsonString.replace("}","");

      int index = 0;
      String part;

      for (int i = 0; i < 14; i++) {
        // Trouver la position de la prochaine virgule
        int commaIndex = jsonString.indexOf(',', index);
      
        // Extraire la sous-chaîne jusqu'à la virgule
        if (commaIndex >= 0) {
          part = jsonString.substring(index, commaIndex);
          index = commaIndex + 1;  // Passer après la virgule
        } else {
          part = jsonString.substring(index);  // Dernière sous-chaîne sans virgule
          index = -1;  // Fin de la boucle
        }
    
        Serial.println(part);  // Afficher la sous-chaîne sur une nouvelle ligne
        liste[i] = part;
      } 
      
      String switchTimerBLT,switchRangeBLT;
      int heureBLT, minuteBLT;
      for (int i = 0; i < 14; i++) {
        liste[i].replace("\"","");
        int startPos = liste[i].indexOf(":") + 1;
        int endPos = liste[i].indexOf(" ", startPos+1);
        String element = liste[i].substring(startPos, endPos);
        Serial.println(element);
        switch(i){
          case 0:
            String anneeBLT = element;
            break;
          case 1:
            int heureBLT = element.toInt(); 
            break;
          case 2:
            String jourBLT = element;
            break;
          case 3:
            int minuteBLT = element.toInt();
            break;
          case 4:
            String moisBLT = element;
            break;
          case 5:
            String NomJourBLT = element;
            break;
          case 6:
            String rangeMaxBLT = element;
            break;
          case 7:
            String rangeMinBLT = element;
            break;
          case 8:
            String switchRangeBLT = element;
            break;
          case 9:
            String switchTimerBLT = element;
            break;
          case 10:
            String timerMatinHeureBLT = element;
            break;
          case 11:
            String timerMatinMinuteBLT = element;
            break;
          case 12:
            String timerSoirHeureBLT = element;
            break;
          case 13:
            String timerSoirMinuteBLT = element;
            break;
        }   
      }
      
      if (switchTimerBLT == "true"){
        selectedOption = 1;
      } else if (switchRangeBLT == "true"){
        selectedOption = 2;
      }else {
        selectedOption = 0;
      }
        
      //numeroDuMois = moisEnNumero(moisJson);
      //rtc.setTime(heureBLT, minuteBLT, 0);
      //rtc.setDate(jourBLT, moisBLT, anneeBLT);
      //EEPROM.write(ADDR_HEURE_TIMER_MATIN, timerMatinHeureBLT);
      //EEPROM.write(ADDR_MINUTE_TIMER_MATIN, timerMatinMinuteBLT);
      //EEPROM.write(ADDR_HEURE_TIMER_SOIR, timerSoirHeureBLT);
      //EEPROM.write(ADDR_MINUTE_TIMER_SOIR, timerSoirMinuteBLT);
      //EEPROM.write(ADDR_SELECTED_OPTION, selectedOption);

      //EEPROM.put(ADDR_RANGE_MIN, rangeMinJson);
      //EEPROM.put(ADDR_RANGE_MAX, rangeMaxJson);
        
      //loadValuesFromEEPROM();
      
    }
  }
}

void sendbluetooth(){

  bool BLTSwitchTimer, BLTSwitchRange;
  
  if (selectedOption == 1){
    BLTSwitchTimer = true;
    BLTSwitchRange = false;
  }else if(selectedOption == 2){
    BLTSwitchTimer = false;
    BLTSwitchRange = true;
  }else{
    BLTSwitchTimer = false;
    BLTSwitchRange = false;
  }

  Serial.print("Heure.");
  Serial.println(heure_now);
  Serial.print("Minute.");
  Serial.println(minute_now);
  Serial.print("NomJour.");
  Serial.println();
  Serial.print("Jour.");
  Serial.println(jour);
  Serial.print("Mois.");
  Serial.println(mois);
  Serial.print("Année.");
  Serial.println(annee);
  Serial.print("SwitchTimer.");
  Serial.println(BLTSwitchTimer);
  Serial.print("SwitchRange.");
  Serial.println(BLTSwitchRange);
  Serial.print("TimerMatinHeure.");
  Serial.println(heure_TimerMatin);
  Serial.print("TimerMatinMinute.");
  Serial.println(minute_TimerMatin);
  Serial.print("TimerSoirHeure.");
  Serial.println(heure_TimerSoir);
  Serial.print("TimerSoirMinute.");
  Serial.println(minute_TimerSoir);
  Serial.print("RangeMin.");
  Serial.println(rangeMin);
  Serial.print("RangeMax.");
  Serial.println(rangeMax);
}
