////////////////////////////////////LIBRERIAS/////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  //a veces la dirección no es 0x3f. Cambie a 0x27 si no funciona.
//Fingerprint libraries
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
int getFingerprintIDez();
SoftwareSerial mySerial(10, 11);  // el pin n.º 2 está ENTRADA del sensor (cable VERDE)/ el pin n.º 3 está FUERA de arduino (cable BLANCO)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
//Real time clock libraries
#include <DS3231.h>
DS3231  rtc(SDA, SCL);// Inicie el DS3231 usando la interfaz de hardware
//Bibliotecas de lectura/escritura de tarjeta SD
#include <SPI.h>
#include <SD.h>
File myFile;
//servo library
#include <Servo.h>
Servo miServo;


/////////////////////////////////////Entradas y salidas////////// ///////////////////////////////////////

int scanner_pin = 13;      //Pin para el botón de escaneo
int agregar_pin_id = 12;    //Pin para el pulsador de agregar nuevo ID
int cerrar_puerta = 9;     //Pin para cerrar el boton de la puerta
int led_verde = 8;      // LED adicionales para etiquetas de puertas abiertas o cerradas
int led_rojo = 7;
int servo = 6;        //Pin for the Servo PWM signal

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////Variables editables//////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
int usuario_principal_ID = 4;                //Cambia este valor si quieres un usuario principal diferente
int puerta_grados_abierto = 180;
int puerta_grados_cerrada = 0;
String archivo_nombre_a_guardar = "blackBox.txt";
//////////////////////////////////////////////////////////////////////////////////////////////////////

//Cambia este valor si quieres un usuario principal diferente
bool scanning = false;
int contador = 0;
int id_agregar_cont = 0;
bool id_agregar = false;
uint8_t num = 1;
bool id_selected = false;
uint8_t id;
bool primer_leer = false;
bool usuario_principal = false;
bool agregar_nuevo_id = false;
bool puerta_bloqueada = true;
char R;

void setup() {
  Serial.begin(57600);        //Iniciar la comunicación en serie para datos de TX RX de huellas dactilares.
  Serial1.begin(57600);
  rtc.begin();                //Inicie el reloj en tiempo real (recuerde configurar el tiempo antes de este código)
  SD.begin(53);               //Inicie el módulo de la tarjeta SD con el pin CS conectado a D53 del Arduino MEGA. Los otros pines son pines SPI
  /*Now we open the new created file, write the data, and close it back*/
  myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);   //Create a new file on the SD card named before
  myFile.print("Door lock system started at ");
  myFile.print(rtc.getTimeStr()); myFile.print(" and day ");  myFile.print(rtc.getDateStr());
  myFile.println(" ");myFile.println(" ");
  myFile.close(); 
   
  lcd.init();                     //Inicie la pantalla LCD con comunicación i2c e imprima texto
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print(" Pulse ESCANEAR ");
  lcd.setCursor(0,1);
  lcd.print("Puerta bloqueada");

  //Define pins as outputs or inputs
  pinMode(scanner_pin,INPUT);  
  pinMode(agregar_pin_id,INPUT);  
  pinMode(cerrar_puerta,INPUT);  
  miServo.attach(servo);
  miServo.write(puerta_grados_cerrada);  //Puerta cerrada
  digitalWrite(led_rojo,HIGH);         //LED rojo encendido, muestra puerta CERRADA
  digitalWrite(led_verde,LOW);        //LED verde apagado
  finger.begin(57600);                // establecer la velocidad de datos para el puerto serie del sensor
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////BUCLE VACÍO////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////


void loop() {
  ////////////Recepciion de datos comunicacion serial/////////////
   if(Serial1.available())
   {
    R=Serial1.read();
    Serial.write(R);
  }
/////////////////////////////////////////BOTÓN DE CIERRE DE LA PUERTA PRESIONADO///////////////////////////////////
  if(digitalRead(cerrar_puerta)||R=='4')
  {
    R=0;
    puerta_bloqueada = true;
    miServo.write(puerta_grados_cerrada);  //Puerta cerrada
  digitalWrite(led_rojo,HIGH);         //LED rojo encendido, muestra puerta CERRADA
  digitalWrite(led_verde,LOW);        //LED verde apagado
    lcd.setCursor(0,0);
    lcd.print("Puerta cerrada!");
    lcd.setCursor(0,1);
    lcd.print("                "); 
    delay(2000);     
    lcd.setCursor(0,0);
    lcd.print(" Pulse ESCANEAR ");
    lcd.setCursor(0,1);
    lcd.print(" -Puerta cerrada- ");
    myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println("puerta cerrada por usuario");
    myFile.close(); 
  }


////////////////////////////////Botón de escaneo presionado/////////////////////////////////////////////////
 if(digitalRead(scanner_pin) && !id_agregar||R=='1'&& !id_agregar)
 {
  R=0;
  myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.println(" -- Intento de abrir de puerta");
  myFile.close(); 
  scanning = true;
  lcd.setCursor(0,0);
  lcd.print("Coloque el dedo");
  lcd.setCursor(0,1);
  lcd.print("ESCANEANDO--------");
 }
  
 while(scanning && contador <= 60)
 {
  getFingerprintID();
  delay(100); 
  contador = contador + 1;
  if(contador == 20)
  {
    lcd.setCursor(0,0);
    lcd.print("Coloque el dedo");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO  ----");
  }

  if(contador == 40)
  {
    lcd.setCursor(0,0);
    lcd.print("Coloque el dedo");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO    --");
  }

  if(contador == 50)
  {
    lcd.setCursor(0,0);
    lcd.print("Coloque el dedo");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO      ");
  }
  if(contador == 59)
  {
    lcd.setCursor(0,0);
    lcd.print("¡tiempo agotado!");
    lcd.setCursor(0,1);
    lcd.print("volver a probar!");
    myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- escaneando tiempo agotado!");
    myFile.close(); 
    delay(2000);
    if(puerta_bloqueada)
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print("Puerta bloqueada");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print(" Puerta abierta ");  
    }
   }
   
 }
 scanning = false;
 contador = 0;
///////////////////////////////FINALIZA LA PARTE CON EL ESCANEO



//////////////////////////////////////////Agregar nuevo botón presionado///////////////////////////////////////////
if(digitalRead(agregar_pin_id) && !id_agregar||R=='2'&& !id_agregar)
 {
  R=0;
  myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.println(" -- intento de agregar nueva id!");
  myFile.close(); 

  agregar_nuevo_id = true;

  lcd.setCursor(0,0);
  lcd.print("     Usuario    ");
  lcd.setCursor(0,1);
  lcd.print("    principal   ");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Ingrese ID   ");
  lcd.setCursor(0,1);
  lcd.print("    principal   ");
  
  while (id_agregar_cont < 40 && !usuario_principal)
  {
    getFingerprintID();
    delay(100);  
    id_agregar_cont = id_agregar_cont+1;
    if(!agregar_nuevo_id)
    {
      id_agregar_cont = 40;
    }
  }
  id_agregar_cont = 0;
  agregar_nuevo_id = false;

  
  if(usuario_principal)
  {
    lcd.setCursor(0,0);
    lcd.print("agregar nuevo ID#");
    lcd.setCursor(0,1);
    lcd.print("a base de datos ");  
    delay(1500);
    print_num(num);  
    id_agregar = true;
    myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- agregar nuevo usuario permiso autorizado");
    myFile.close(); 
  }
  else
  {
    lcd.setCursor(0,0);
    lcd.print("     ERROR!    ");
    delay(1000);
    lcd.clear();
    lcd.print(" solo principal ");
    lcd.setCursor(0,1);
    lcd.print("puede agregar ID");  
    delay(2000); 
    if(puerta_bloqueada)
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print("Puerta bloqueada");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print(" Puerta abierta ");  
    }
    id_agregar = false;
    myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.println(" -- El usuario no tiene permiso paa añadir un nuevo usuario");
    myFile.close(); 
  }
 }

if(digitalRead(scanner_pin) && id_agregar||R=='3'&& id_agregar)
 {  
  id=num;
  while (!  getFingerprintEnroll() );
  R=0;
  id_agregar = false;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("nueva ID salvada");
  lcd.setCursor(0,1);
  lcd.print("como ID#"); 
  lcd.setCursor(11,1);
  lcd.print(id);  
  delay(3000);
  if(puerta_bloqueada)
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print("Puerta bloqueada");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print(" Puerta abierta ");  
    }
  agregar_nuevo_id = false;
  usuario_principal = false;
  id_agregar = false;
 
  
 }

if(digitalRead(agregar_pin_id) && id_agregar||R=='2'&& id_agregar)
 {
  R=0;
  num = num + 1;
  if(num > 16)
  {
    num=1;
  }
  print_num(num);  
 }







}//end of void


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////


/*This function will print the ID numbers when adding new ID*/
void print_num(uint8_t)
{
  if (num == 1)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(">1  2   3   4   ");  
    delay(500);
  }
  if (num == 2)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 1 >2   3   4   ");  
    delay(500);
  }
  if (num == 3)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 1  2  >3   4   ");  
    delay(500);
  }
  if (num == 4)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 1  2   3  >4   ");  
    delay(500);
  }
  if (num == 5)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(">5  6   7   8   ");  
    delay(500);
  }
  if (num == 6)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 5 >6   7   8   ");  
    delay(500);
  }
  if (num == 7)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 5  6  >7   8   ");   
    delay(500);
  }
  if (num == 8)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 5  6   7  >8   ");   
    delay(500);
  }
  if (num == 9)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(">9  10  11  12  ");  
    delay(500);
  }
  if (num == 10)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 9 >10  11  12  ");  
    delay(500);
  }
  if (num == 11)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 9  10 >11  12  ");  
    delay(500);
  }
  if (num == 12)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 9  10  11 >12  ");  
    delay(500);
  }
  if (num == 13)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(">13  14  15  16 ");  
    delay(500);
  }
  if (num == 14)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 13 >14  15  16 ");  
    delay(500);
  }
  if (num == 15)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 13  14 >15  16 ");  
    delay(500);
  }
  if (num == 16)
  {
    lcd.setCursor(0,0);
    lcd.print("Elegir ID numero");
    lcd.setCursor(0,1);
    lcd.print(" 13  14  15 >16 ");  
    delay(500);
  }
}









/*Esta función leerá la huella digital colocada en el sensor*/

uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
    break;
    case FINGERPRINT_NOFINGER: return p;
    case FINGERPRINT_PACKETRECIEVEERR: return p;
    case FINGERPRINT_IMAGEFAIL: return p;
    default: return p; 
  }
// OK success!

  p = finger.image2Tz();
  switch (p) 
  {
    case FINGERPRINT_OK: break;    
    case FINGERPRINT_IMAGEMESS: return p;    
    case FINGERPRINT_PACKETRECIEVEERR: return p;  
    case FINGERPRINT_FEATUREFAIL: return p;  
    case FINGERPRINT_INVALIDIMAGE: return p;    
    default: return p;
  }
// OK converted!

p = finger.fingerFastSearch();

if (p == FINGERPRINT_OK)
{
  scanning = false;
  contador = 0;
  if(agregar_nuevo_id)
  {
    if(finger.fingerID == usuario_principal_ID)
    {
      usuario_principal = true;
      id_agregar = false;
    }
    else
    {
      agregar_nuevo_id = false;
      usuario_principal = false;
      id_agregar = false;
    }
    
  }
  else
  {
  miServo.write(puerta_grados_abierto); //grados para que la puerta esté abierta
  digitalWrite(led_rojo,LOW);       //LED rojo apagado
  digitalWrite(led_verde,HIGH);    //LED verde encendido, muestra puerta ABIERTA
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Usuario hallado");
  
  lcd.setCursor(0,1);
  lcd.print(" ID: #");
  
  lcd.setCursor(6,1);
  lcd.print(finger.fingerID);

  lcd.setCursor(9,1);
  lcd.print("%: ");

  lcd.setCursor(12,1);
  lcd.print(finger.confidence);

  myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" Usuario encontrado por ID# "); myFile.print(finger.fingerID);
  myFile.print("con confiabilidad:");   myFile.print(finger.confidence); myFile.println(" - door open");
  myFile.close(); 
  puerta_bloqueada = false;
  delay(4000);
  lcd.setCursor(0,0);
  lcd.print(" Pulse ESCANEAR ");
  lcd.setCursor(0,1);
  lcd.print(" Puerta abierta ");
  delay(50);
  }
}//end finger OK

else if(p == FINGERPRINT_NOTFOUND)
{
  scanning = false;
  id_agregar = false;
  contador = 0;
  lcd.setCursor(0,0);
  lcd.print(" No encontrada ");
  lcd.setCursor(0,1);
  lcd.print("Intente de nuevo! ");
  agregar_nuevo_id = false;
  usuario_principal = false;  

  myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" -- No se encontraron coencidencias con alguna ID. el estado de la puerta se mantiene"); 
  myFile.close();

  
  delay(2000);
  if(puerta_bloqueada)
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print("Puerta bloqueada");   
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print(" Pulse ESCANEAR ");
      lcd.setCursor(0,1);
      lcd.print(" Puerta abierta ");  
    }
  delay(2);
  return p;
}//end finger error
}// returns -1 if failed, otherwise returns ID #


int getFingerprintIDez() {
uint8_t p = finger.getImage();
if (p != FINGERPRINT_OK) return -1;
p = finger.image2Tz();
if (p != FINGERPRINT_OK) return -1;
p = finger.fingerFastSearch();
if (p != FINGERPRINT_OK) return -1;
// found a match!
return finger.fingerID;
}









//////////////////////////////////////////










/*This function will add new ID to the database*/
uint8_t getFingerprintEnroll() {

  int p = -1;
  if(!primer_leer)
  {
  lcd.setCursor(0,0);
  lcd.print(" Agregar nuevo ");
  lcd.setCursor(0,1);
  lcd.print("como ID#");
  lcd.setCursor(11,1);
  lcd.print(id);
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Ponga dedo   ");  
  }
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" huella tomada! ");
      lcd.setCursor(0,1);
      lcd.print("                ");  
      delay(100);
      primer_leer = true;
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0,0);
      lcd.print(" Agregar nuevo ");
      lcd.setCursor(0,1);
      lcd.print("como ID#");
      lcd.setCursor(11,1);
      lcd.print(id);
      delay(2000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("   Ponga dedo   ");   
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("    -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("  -De huella!  ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("    ERROR    ");
      lcd.setCursor(0,1);
      lcd.print(" -Desconocido!  ");
      delay(1000);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print("     huella      ");
      lcd.setCursor(0,1);
      lcd.print("   convertida!   ");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.setCursor(0,0);
      lcd.print(" huella difusa! ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(1000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
      delay(1000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.setCursor(0,0);
      lcd.print("no hay rasgos de");
      lcd.setCursor(0,1);
      lcd.print("huella dactilar");
      delay(1000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.setCursor(0,0);
      lcd.print("no hay rasgos de");
      lcd.setCursor(0,1);
      lcd.print("huella dactilar");
      delay(1000);
      return p;
    default:
      lcd.setCursor(0,0);
      lcd.print("    ERROR    ");
      lcd.setCursor(0,1);
      lcd.print(" -Desconocido!  ");
      delay(1000);
      return p;
  }
  
  lcd.setCursor(0,0);
  lcd.print(" Remover huella!");
  lcd.setCursor(0,1);
  lcd.print("                ");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  lcd.setCursor(0,1);
  lcd.print("ID# ");
  lcd.setCursor(4,1);
  lcd.print(id);
  p = -1;
  lcd.setCursor(0,0);
  lcd.print(" poner de nuevo ");
  lcd.setCursor(0,1);
  lcd.print(" la misma huella");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" huella tomada! ");
      lcd.setCursor(0,1);
      lcd.print("                "); 
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0,0);
      lcd.print(" poner de nuevo ");
      lcd.setCursor(0,1);
      lcd.print(" la misma huella");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("    -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("  -De huella!  ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("    -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print(" -Desconocido! ");
      delay(1000);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print("     huella      ");
      lcd.setCursor(0,1);
      lcd.print("   convertida!   ");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.setCursor(0,0);
      lcd.print(" huella difusa! ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(1000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
      delay(1000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.setCursor(0,0);
      lcd.print("no hay rasgos de");
      lcd.setCursor(0,1);
      lcd.print("huella dactilar");
      delay(1000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
      delay(1000);
      return p;
    default:
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print(" -Desconocido!  ");
      delay(1000);
      return p;
  }
  
  // OK converted!
 lcd.setCursor(0,0);
 lcd.print(" creando modelo ");
 lcd.setCursor(0,1);
 lcd.print("for ID# ");
 lcd.setCursor(8,1);
 lcd.print(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0,0);
    lcd.print(" Huella Digital ");
    lcd.setCursor(0,1);
    lcd.print("   Encontrada   ");
    delay(1000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print("De Comunicacion!");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    lcd.setCursor(0,0);
    lcd.print("huella digital");
    lcd.setCursor(0,1);
    lcd.print("no encontrada");
    delay(1000);
    return p;
  } else {
      lcd.setCursor(0,0);
      lcd.print("     -ERROR     ");
      lcd.setCursor(0,1);
      lcd.print(" -Desconocido!  ");
    delay(1000);
    return p;
  }   
  
  lcd.setCursor(0,1);
  lcd.print("ID# ");
  lcd.setCursor(4,1);
  lcd.print(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0,0);
    lcd.print("    Guardada    ");
    lcd.setCursor(0,1);
    lcd.print("                ");
    myFile = SD.open(archivo_nombre_a_guardar, FILE_WRITE);
    myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
    myFile.print(" -- Nueva huella digital guardada para el ID# "); myFile.println(id);
    myFile.close(); 
    delay(1000);
    primer_leer = false;
    id_agregar = false;
    
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.setCursor(0,0);
    lcd.print("     -ERROR     ");
    lcd.setCursor(0,1);
    lcd.print("De Comunicacion!");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    lcd.setCursor(0,0);
    lcd.print("no se guardo");
    lcd.setCursor(0,1);
    lcd.print("en esa posicion");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    lcd.setCursor(0,0);
    lcd.print("Error al escribir");
    lcd.setCursor(0,1);
    lcd.print("  en la flash  ");
    delay(1000);
    return p;
  } else {
    lcd.setCursor(0,0);
    lcd.print("     -ERROR     ");
    lcd.setCursor(0,1);
    lcd.print(" -Desconocido!  ");    
    delay(1000);
    return p;
  }   
}
