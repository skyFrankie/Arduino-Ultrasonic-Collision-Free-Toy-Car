#include <Arduino_FreeRTOS.h>
#include <Servo.h>
#include <semphr.h>
#include <queue.h>
const int leftForward = 8;
const int leftBackward = 3;
const int rightForward = 4;
const int rightBackward = 5;
#define echoPin 6 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 7 //attach pin D3 Arduino to pin Trig of HC-SR04
int buttonpin = 2; //Define pin 8 for button interrupt
int servoPin = 9;// define servo motor signal at pin9
int interruptLEDpin = 18; //Define Blinking LED for interrupt at pin19(A4)
int ledPin = 19; //Define Blinking LED all the time at pin19(A5)
int intercount = 0;
int tempcount;
int dir = 0;
int servoangle[3] = {15,90,165};
int temp = 0;

SemaphoreHandle_t mutex_v; //Mutex variable
SemaphoreHandle_t interruptSemaphore; //Binary Semaphore
QueueHandle_t queue_1;
QueueHandle_t queue_2;
QueueHandle_t queue_3;
int leftdist =0;
int frontdist =0;
int rightdist =0;
// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement
int distance2; // variable for the distance measurement
int direct;
TaskHandle_t TaskHandle_1;//Blinking LED	
TaskHandle_t TaskHandle_2;//Interrupt LED
TaskHandle_t TaskHandle_3;//Transform
TaskHandle_t TaskHandle_4;//TaskUrgent
TaskHandle_t TaskHandle_5;//regular ultrasonic
Servo Servo1;

void TaskLed(void *pvParameter);
void TaskBlink1(void *pvParameter);
void Movement(void *pvParameter);
void UltraSonic(void *pvParameter);
void TaskUrgent(void *pvParameter);
void Transform(void *pvParameters);

//solve the debouncing problem
long debouncing_time = 150; 
volatile unsigned long last_micros;
void debounceInterrupt() {
if((long)(micros() - last_micros) >= debouncing_time * 1000) {
interruptHandler();
last_micros = micros();
}
}
void interruptHandler() {
xSemaphoreGiveFromISR(interruptSemaphore, NULL);
}

void setup() {
  Serial.begin(9600);
  pinMode(buttonpin, INPUT_PULLUP); //Activate buttopin at pullup state
  Servo1.attach(servoPin);
  Servo1.write(45);
  delay(1000);
  Servo1.write(135);
  delay(1000);
  Servo1.write(90);
  pinMode(leftForward , OUTPUT);
  pinMode(leftBackward , OUTPUT);
  pinMode(rightForward , OUTPUT);
  pinMode(rightBackward , OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  pinMode(ledPin, OUTPUT);//Sets the LED as an OUTPUT
  pinMode(interruptLEDpin, OUTPUT);//Sets the LED as an OUTPUT
  //xTaskCreate(TaskBlink1,"LEDblink",128,NULL,0,&TaskHandle_1);
  xTaskCreate(Movement, "move", 128, NULL,3,&TaskHandle_1);
  vTaskSuspend(TaskHandle_1);
  xTaskCreate(TaskLed, "LedInterrpt", 64, NULL,1,&TaskHandle_2);
  xTaskCreate(UltraSonic, "StartUltraSonic", 128, NULL, 5,&TaskHandle_5);
  xTaskCreate(TaskUrgent, "Read&blink faster", 64, NULL, 2, &TaskHandle_4);
  xTaskCreate(Transform, "channel 2", 64, NULL,4, &TaskHandle_3);
  vTaskSuspend(TaskHandle_3);
  mutex_v = xSemaphoreCreateMutex();//set up Mutex 
  interruptSemaphore = xSemaphoreCreateBinary(); //Setup binary semaphore
  if (interruptSemaphore != NULL) {
    attachInterrupt(digitalPinToInterrupt(buttonpin), debounceInterrupt,LOW);
  }
  queue_1 = xQueueCreate(5, sizeof(int));
  queue_2 = xQueueCreate(3, sizeof(int));
  queue_3 = xQueueCreate(1, sizeof(int));
  vTaskStartScheduler();
}
void loop() {}

//............................................................................................
void TaskLed(void *pvParameters)
{
while(1) {
if (xSemaphoreTake(interruptSemaphore, portMAX_DELAY) == pdPASS) {
  intercount ++;
  if (intercount%2 == 1){
  Serial.println("Interrupt");
  xSemaphoreTake(mutex_v, portMAX_DELAY); 
  for(int k =1; k<25;k++){
  digitalWrite(interruptLEDpin,HIGH);
  vTaskDelay( 20/ portTICK_PERIOD_MS );
  digitalWrite(interruptLEDpin,LOW);  
  vTaskDelay( 20/ portTICK_PERIOD_MS );
  }
  xSemaphoreGive(mutex_v); 
  digitalWrite(interruptLEDpin,HIGH);
  vTaskResume(TaskHandle_3);
  vTaskResume(TaskHandle_1);
  //vTaskDelete(TaskHandle_4);
  
    }
  else{
   Serial.println("Interrupt");
   xSemaphoreTake(mutex_v, portMAX_DELAY); 
   for(int k =1; k<25;k++){
   digitalWrite(interruptLEDpin,HIGH);
   vTaskDelay( 20/ portTICK_PERIOD_MS );
   digitalWrite(interruptLEDpin,LOW);  
   vTaskDelay( 20/ portTICK_PERIOD_MS );
   }
   xSemaphoreGive(mutex_v); 
   digitalWrite(interruptLEDpin,LOW);
   vTaskSuspend(TaskHandle_3);
   vTaskSuspend(TaskHandle_1);
   //xTaskCreate(TaskUrgent, "Read&blink faster", 128, NULL, 2, &TaskHandle_4);
  }
}  
}
}
//.....................................................................................................
//void TaskBlink1(void *pvParameters)  {
  //while(1)         //$$$$$$$$$$$$ Can be used as interrupt point $$$$$$$$$$$$$$$
  //{
  //  digitalWrite(ledPin, HIGH);   
  //  vTaskDelay( 1000/ portTICK_PERIOD_MS ); 
   // digitalWrite(ledPin, LOW);    
   // vTaskDelay( 1000/ portTICK_PERIOD_MS );
//  }
//}
//........................................................................................................
void TaskUrgent(void * pvParameters) {
  int distance = 0;
  int distance2 = 0;
  //int counter = 1;
  int direct = 0;
  while(1){
     if (xQueueReceive(queue_1, &distance, 1000) == pdPASS) {
      if (intercount%2 == 0){
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
      if (distance <=5){
        //vTaskSuspend(TaskHandle_1);
        digitalWrite(ledPin, HIGH);   
        vTaskDelay( 50/ portTICK_PERIOD_MS ); 
        digitalWrite(ledPin, LOW);    
        vTaskDelay( 50/ portTICK_PERIOD_MS );
      }
      else{
        //vTaskResume(TaskHandle_1);
      }
     }       
    }
   }
  }
void Transform(void *pvParameters){
int distance = 0;
int distanc2 = 0;
int direct = 0;
for(;;){
if (leftdist != 0 && frontdist != 0 && rightdist != 0){
          vTaskDelay( 500 / portTICK_PERIOD_MS );
          int x = max(leftdist,frontdist);
          int y = max(leftdist,rightdist);
          //Serial.print(frontdist );
          //Serial.print('\n');
          //Serial.print(leftdist );
          //Serial.print('\n');
          //Serial.print(rightdist );
          //Serial.print('\n');
          if(frontdist <20 && y<25){//Move backward for 2sec
            xSemaphoreTake(mutex_v, portMAX_DELAY);
            direct = 0;
            xQueueSend(queue_3, &direct, portMAX_DELAY);
            leftdist=0;
            frontdist=0;
            rightdist=0;
            xSemaphoreGive(mutex_v);  
            vTaskDelay( 2600/ portTICK_PERIOD_MS );            
          }
          else if(frontdist>= 30 && y>=25){ //Move forward for 2sec
            direct = 1;
            xSemaphoreTake(mutex_v, portMAX_DELAY);
            xQueueSend(queue_3, &direct, portMAX_DELAY);
            leftdist=0;
            frontdist=0;
            rightdist=0;
            xSemaphoreGive(mutex_v);    
            vTaskDelay( 2600/ portTICK_PERIOD_MS );            
            }
          else if(frontdist>= 30 && y<25){ //Move forward for 2sec
            direct = 1;
            xSemaphoreTake(mutex_v, portMAX_DELAY);
            xQueueSend(queue_3, &direct, portMAX_DELAY);
            leftdist=0;
            frontdist=0;
            rightdist=0;
            xSemaphoreGive(mutex_v);    
            vTaskDelay( 2600/ portTICK_PERIOD_MS );            
            }
          else if(y == leftdist){ 
            direct = 3;
            xSemaphoreTake(mutex_v, portMAX_DELAY);
            xQueueSend(queue_3, &direct, portMAX_DELAY);
            leftdist=0;
            frontdist=0;
            rightdist=0;
            xSemaphoreGive(mutex_v);  
            vTaskDelay( 2600/ portTICK_PERIOD_MS );            
            }//turn left
          else if(y == rightdist){
            direct = 2;
            xSemaphoreTake(mutex_v, portMAX_DELAY);
            xQueueSend(queue_3, &direct, portMAX_DELAY);
            leftdist=0;
            frontdist=0;
            rightdist=0;
            xSemaphoreGive(mutex_v);  
            vTaskDelay( 2600/ portTICK_PERIOD_MS );           
            }//turn right
          }
if (xQueueReceive(queue_2, &distance2, portMAX_DELAY) == pdPASS) {
       temp = distance2 + temp;
       if (tempcount%3 == 0 && dir == 2){
         xSemaphoreTake(mutex_v, portMAX_DELAY);
         frontdist = temp/3;
         temp =0;
         xSemaphoreGive(mutex_v);  
         }
       else if (tempcount%3 == 0 && dir == 1){
         xSemaphoreTake(mutex_v, portMAX_DELAY);
         rightdist = temp/3;
         temp =0;
         xSemaphoreGive(mutex_v);          
         }
       else if (tempcount%3 == 0 && dir == 0){
         xSemaphoreTake(mutex_v, portMAX_DELAY);        
         leftdist = temp/3;
         temp =0;     
         xSemaphoreGive(mutex_v);          
         }    
      }    
}
}
//................................................................................................
void UltraSonic(void *pvParameters) {
  for(;;){
  if (intercount%2 == 1){
    if (tempcount ==3){
        dir++;
        //Serial.println("Turn");
        tempcount=0;
        }
    if (dir == 3){
      dir = 0;
      Servo1.write(servoangle[dir]);
      vTaskDelay( 3500/ portTICK_PERIOD_MS );  
      }
    Servo1.write(servoangle[dir]);
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); 
    for(int y  =0;y<3;y++){
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      digitalWrite(trigPin, LOW);
      vTaskDelay( 2 / portTICK_PERIOD_MS );
      // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
      digitalWrite(trigPin, HIGH);
      vTaskDelay( 10 / portTICK_PERIOD_MS );
      digitalWrite(trigPin, LOW);
      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);
      // Calculating the distance
      distance2 = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
      //Serial.print("From Queue_2 ");
      //Serial.print(distance2);
      //Serial.println("cm");      
      xQueueSend(queue_2, &distance2, portMAX_DELAY);
      tempcount ++;
      }
  }
  else if (intercount%2 == 0){
  digitalWrite(trigPin, LOW);  
  vTaskDelay( 2 / portTICK_PERIOD_MS );
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  vTaskDelay( 10 / portTICK_PERIOD_MS );
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  xQueueSend(queue_1, &distance, portMAX_DELAY);
  Serial.println("From Queue_1");
  vTaskDelay( 200/ portTICK_PERIOD_MS );
    }
  }
}


//..............................................................................................................
void Movement(void *pvParameter){
  while(1){
    if (xQueueReceive(queue_3, &direct, portMAX_DELAY) == pdPASS) {
    Serial.println(direct);
    if(direct == 0){ //Move backward for 2sec
      vTaskDelay( 100/ portTICK_PERIOD_MS );      
      digitalWrite(leftForward , HIGH);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , HIGH);
      digitalWrite(rightBackward , LOW);      
      vTaskDelay(900/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward ,LOW);
      vTaskDelay(1200/ portTICK_PERIOD_MS );
    }
    else if(direct == 1){ //Move forward for 2sec
      vTaskDelay( 100/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , HIGH);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward , HIGH);
      vTaskDelay(900/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward ,LOW);
      vTaskDelay(1200/ portTICK_PERIOD_MS );
    }
    else if(direct == 2){ //turn left
      vTaskDelay( 100/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , HIGH);
      digitalWrite(rightForward , HIGH);
      digitalWrite(rightBackward , LOW);
      vTaskDelay(700/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward ,LOW);
      vTaskDelay(1000/ portTICK_PERIOD_MS );
      vTaskDelay(400/ portTICK_PERIOD_MS );
    }
    else if(direct == 3){ //turn right
      vTaskDelay( 100/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , HIGH);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward , HIGH);
      vTaskDelay(700/ portTICK_PERIOD_MS );
      digitalWrite(leftForward , LOW);
      digitalWrite(leftBackward , LOW);
      digitalWrite(rightForward , LOW);
      digitalWrite(rightBackward ,LOW);
      vTaskDelay(1000/ portTICK_PERIOD_MS );
      vTaskDelay(400/ portTICK_PERIOD_MS );
    }
  }
 }
}



//void Servomove(void *pvParameter){
  //while(1){
    //Servo1.write(15);
   // vTaskDelay(2000/ portTICK_PERIOD_MS);
   // Servo1.write(90);
  //  vTaskDelay(2000/ portTICK_PERIOD_MS);
  //  Servo1.write(165);
  //  vTaskDelay(2000/ portTICK_PERIOD_MS);
 // }
//}
