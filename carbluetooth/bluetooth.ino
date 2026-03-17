#define DIR_RIGHT 4
#define SPEED_RIGHT 5

#define DIR_LEFT 7
#define SPEED_LEFT 6

#define LEFT_FORWARD LOW
#define LEFT_BACKWARD HIGH

#define RIGHT_FORWARD LOW
#define RIGHT_BACKWARD HIGH

#define BT_SPEED 200 

char command; 

void setup() {
  Serial.begin(9600);

  pinMode(DIR_RIGHT, OUTPUT);
  pinMode(SPEED_RIGHT, OUTPUT);
  pinMode(DIR_LEFT, OUTPUT);
  pinMode(SPEED_LEFT, OUTPUT);

  stopCar();
}

void loop() {
  if (Serial.available() > 0) {
    command = Serial.read(); 
    
    switch (command) {
      case 'F': 
        forward(BT_SPEED);
        break;
      case 'B': 
        backward(BT_SPEED);
        break;
      case 'L': 
        rotate_left(BT_SPEED);
        break;
      case 'R': 
        rotate_right(BT_SPEED);
        break;
      case 'S': 
        stopCar();
        break;
    }
  }
}

void move(bool left_dir, int left_speed, bool right_dir, int right_speed) {
  digitalWrite(DIR_LEFT, left_dir);
  digitalWrite(DIR_RIGHT, right_dir);
  analogWrite(SPEED_LEFT, left_speed);
  analogWrite(SPEED_RIGHT, right_speed);
}

void forward(int speed) {
  move(LEFT_FORWARD, speed, RIGHT_FORWARD, speed);
}

void backward(int speed) {
  move(LEFT_BACKWARD, speed, RIGHT_BACKWARD, speed);
}

void rotate_left(int speed) {
  move(LEFT_BACKWARD, speed, RIGHT_FORWARD, speed);
}

void rotate_right(int speed) {
  move(LEFT_FORWARD, speed, RIGHT_BACKWARD, speed);
}

void stopCar() {
  move(LEFT_FORWARD, 0, RIGHT_FORWARD, 0);
}
