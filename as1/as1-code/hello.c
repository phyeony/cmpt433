#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define FILE_PATH_SIZE 35
#define LED_FILE_PATH_SIZE 60

static long long getTimeInMs(void)
{
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  long long seconds = spec.tv_sec;
  long long nanoSeconds = spec.tv_nsec;
  long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
  return milliSeconds;
}

static void sleepForMs(long long delayInMs)
{
  const long long NS_PER_MS = 1000 * 1000;
  const long long NS_PER_SECOND = 1000000000;
  long long delayNs = delayInMs * NS_PER_MS;
  int seconds = delayNs / NS_PER_SECOND;
  int nanoseconds = delayNs % NS_PER_SECOND;
  struct timespec reqDelay = {seconds, nanoseconds};
  int response = nanosleep(&reqDelay, (struct timespec *)NULL);
  if (response != 0)
  {
    printf("ERROR: Sleep failed...\n");
    exit(1);
  }
}

static void runCommand(char *command)
{
  // Execute the shell command (output into pipe)
  FILE *pipe = popen(command, "r");
  // Ignore output of the command; but consume it
  // so we don't get an error when closing the pipe.
  char buffer[1024];
  while (!feof(pipe) && !ferror(pipe))
  {
    if (fgets(buffer, sizeof(buffer), pipe) == NULL)
      break;
    // printf("--> %s", buffer); // Uncomment for debugging
  }
  // Get the exit code from the pipe; non-zero is an error:
  int exitCode = WEXITSTATUS(pclose(pipe));
  if (exitCode != 0)
  {
    perror("Unable to execute command:");
    printf(" command: %s\n", command);
    printf(" exit code: %d\n", exitCode);
  }
}

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NO_DIRECTION
} JoystickDirection;

bool JOYSTICK_isPressed(JoystickDirection direction)
{
  // Referred https://www.geeksforgeeks.org/c-program-to-read-contents-of-whole-file/
  // For reading a value from a file
  FILE *file;
  int gpioVal;
  char filePath[FILE_PATH_SIZE];

  int gpioNum;
  switch (direction)
  {
  case UP:
    gpioNum = 26;
    break;
  case DOWN:
    gpioNum = 46;
    break;
  case LEFT:
    gpioNum = 65;
    break;
  case RIGHT:
    gpioNum = 47;
    break;
  default:
    return false;
  }
  snprintf(filePath, FILE_PATH_SIZE, "/sys/class/gpio/gpio%d/value", gpioNum);

  // Opening file in reading mode
  file = fopen(filePath, "r");

  if (NULL == file)
  {
    printf("ERROR OPENING %s.", filePath);
    exit(1);
  }

  fscanf(file, "%d", &gpioVal);
  // Closing the file
  fclose(file);
  return gpioVal == 0;
}

void JOYSTICK_setup()
{
  runCommand("config-pin p8.14 gpio | config-pin p8.15 gpio | config-pin p8.16 gpio | config-pin p8.18 gpio");
}

void LEDS_setTriggersToNone()
{
  FILE *file;
  char filePath[LED_FILE_PATH_SIZE];

  for (int i = 0; i < 4; i++)
  {
    snprintf(filePath, LED_FILE_PATH_SIZE, "/sys/class/leds/beaglebone:green:usr%d/trigger", i);
    // Opening file in writing mode
    file = fopen(filePath, "w");

    if (NULL == file)
    {
      printf("ERROR OPENING %s.", filePath);
      exit(1);
    }
    int charWritten = fprintf(file, "none");

    if (charWritten <= 0)
    {
      printf("ERROR WRITING DATA \n");
      exit(1);
    }
    // Closing the file
    fclose(file);
  }
}

void LEDS_setBrightness(int num, char *value)
{
  FILE *file;
  char filePath[LED_FILE_PATH_SIZE];

  snprintf(filePath, LED_FILE_PATH_SIZE, "/sys/class/leds/beaglebone:green:usr%d/brightness", num);
  // Opening file in writing mode
  file = fopen(filePath, "w");

  if (NULL == file)
  {
    printf("ERROR OPENING %s.", filePath);
    exit(1);
  }
  int charWritten = fprintf(file, value);

  if (charWritten <= 0)
  {
    printf("ERROR WRITING DATA \n");
    exit(1);
  }
  // Closing the file
  fclose(file);
}

void LEDS_blinkAll(int hz, float seconds)
{
  long long startTime = getTimeInMs();
  while (getTimeInMs() - startTime <= seconds * 1000)
  {
    LEDS_setBrightness(0, "1");
    LEDS_setBrightness(1, "1");
    LEDS_setBrightness(2, "1");
    LEDS_setBrightness(3, "1");
    sleepForMs(1000 / (2 * hz));
    LEDS_setBrightness(0, "0");
    LEDS_setBrightness(1, "0");
    LEDS_setBrightness(2, "0");
    LEDS_setBrightness(3, "0");
    sleepForMs(1000 / (2 * hz));
  }
}

void LEDS_cleanup()
{
  LEDS_setBrightness(0, "0");
  LEDS_setBrightness(1, "0");
  LEDS_setBrightness(2, "0");
  LEDS_setBrightness(3, "0");
}

int main(int argc, char *args[])
{
  printf("Welcome to the game!\n");
  printf("When the LEDs light up, press the joystick in that direction!(Press left or right to exit) \n");

  // set up joystick and leds
  JOYSTICK_setup();
  LEDS_setTriggersToNone();

  long long bestRecord;
  srand(time(NULL));

  while (1)
  {
    // Step 1
    printf("Get Ready...\n");
    LEDS_setBrightness(0, "0");
    LEDS_setBrightness(1, "1");
    LEDS_setBrightness(2, "1");
    LEDS_setBrightness(3, "0");

    // Step 2
    long long randomTime = (long long)(rand() % 2501 + 500);
    sleepForMs(randomTime);

    // Step 3
    if (JOYSTICK_isPressed(UP) || JOYSTICK_isPressed(DOWN))
    {
      printf("Pressed too soon! Try again...\n");
      continue;
    }
    if (JOYSTICK_isPressed(LEFT) || JOYSTICK_isPressed(RIGHT))
    {
      printf("Left or Right was pressed. Exiting the program...\n");
      LEDS_cleanup();
      exit(1);
    }

    // Step 4
    int random = rand() % 2;
    JoystickDirection chosenDirection, unchosenDirection;
    if (random == 0)
    {
      chosenDirection = UP;
      unchosenDirection = DOWN;
      printf("Press Up Now!\n");
    }
    else
    {
      chosenDirection = DOWN;
      unchosenDirection = UP;
      printf("Press Down Now!\n");
    }

    // Step 5
    LEDS_setBrightness(1, "0");
    LEDS_setBrightness(2, "0");
    if (chosenDirection == UP)
    {
      LEDS_setBrightness(0, "1");
    }
    else
    {
      LEDS_setBrightness(3, "1");
    }

    // Step 6
    long long startTime = getTimeInMs();
    while (1)
    {
      long long responseTime = getTimeInMs() - startTime;

      if (responseTime >= 5000)
      {
        printf("Response time took over 5 seconds. Exiting the program...\n");
        exit(1);
      }

      // Step 7
      //  a)
      if (JOYSTICK_isPressed(chosenDirection))
      {
        responseTime = getTimeInMs() - startTime;
        printf("Correct!\n");
        if (responseTime < bestRecord)
        {
          printf("New best time!\n");
          bestRecord = responseTime;
        }
        printf("Your reaction time was %lldms; best so far in game is %lldms.\n", responseTime, bestRecord);
        LEDS_blinkAll(4, 0.5);
        break;
      }
      // b)
      if (JOYSTICK_isPressed(unchosenDirection))
      {
        printf("Incorrect.\n");
        LEDS_blinkAll(10, 1);
        break;
      }

      // c)
      if (JOYSTICK_isPressed(LEFT) || JOYSTICK_isPressed(RIGHT))
      {
        printf("Left or Right was pressed. Exiting the program...\n");
        LEDS_cleanup();
        exit(1);
      }
    }
  }
  return 0;
}