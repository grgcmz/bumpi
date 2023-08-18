#include <netdb.h>      /* network database library */
#include <sys/socket.h> /* sockets */
#include <arpa/inet.h>  /* address conversions */
#include <unistd.h>
#include <string.h> /* memset, strlen */
#include <stdio.h>  /* printf */

// sbgCommonLib headers
#include <sbgCommon.h>
#include <version/sbgVersion.h>

// sbgECom headers
#include <sbgEComLib.h>

#define BUFSIZE (10 * 1024) /* size of buffer, max 64 KByte */
#define SBG_DEVICENAME ("/dev/ttyUSB0")
#define SBG_BAUDRATE (115200)

static unsigned char buf[BUFSIZE];    /* receive buffer */
const char *host = "192.168.135.237"; /* IP of host */
// const char *host = "127.0.0.1";  /* IP of localhost */
const int port = 1235;             /* port to be used */
static struct sockaddr_in myaddr;  /* our own address */
static struct sockaddr_in remaddr; /* remote address */
static int fd, i;
static socklen_t slen;

const char *testmsg = "test!";

struct Message_s
{
  /* SBG_ECOM_LOG_UTC_TIME */
  uint16_t year;
  int8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  int32_t nanosecond;

  /* SBG_ECOM_LOG_IMU_DATA */
  float accel[3];
  float gyro[3];
  float temperature;
  float deltaVelocity[3];
  float deltaAngle[3];

  /* SBG_ECOM_LOG_GPS1_POS */
  double altitude;
  double latitude;
  double longitude;

} message;

static void printMessage(const struct Message_s *msg) {
    printf("\033[2J"); // Clear the screen
    printf("\033[H");  // Move cursor to top-left

    printf("Received Message:\n");

    printf("Date: %d-%02d-%02d %02d:%02d:%02d.%09d\n",
           msg->year, msg->month, msg->day,
           msg->hour, msg->minute, msg->second, msg->nanosecond);

    printf("Accelerometer: %f %f %f\n", msg->accel[0], msg->accel[1], msg->accel[2]);
    printf("Gyroscope: %f %f %f\n", msg->gyro[0], msg->gyro[1], msg->gyro[2]);
    printf("Temperature: %f\n", msg->temperature);
    printf("Delta Velocity: %f %f %f\n", msg->deltaVelocity[0], msg->deltaVelocity[1], msg->deltaVelocity[2]);
    printf("Delta Angle: %f %f %f\n", msg->deltaAngle[0], msg->deltaAngle[1], msg->deltaAngle[2]);

    printf("Altitude: %f\n", msg->altitude);
    printf("Latitude: %f\n", msg->latitude);
    printf("Longitude: %f\n", msg->longitude);

    fflush(stdout); // Flush output
}

static int udp_init(void)
{
  slen = sizeof(remaddr);
  int recvlen; /* # bytes in acknowledgment message */

  char ipBuf[64]; /* in case 'host' is a hostname and not an IP address, this will hold the IP address */

  /* create a socket */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    printf("socket created\n");
  }

  /* bind it to all local addresses and pick any port number */
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

  if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
  {
    perror("bind failed");
    return -1;
  }

  /* now define remaddr, the address to whom we want to send messages */
  /* For convenience, the host address is expressed as a numeric IP address */
  /* that we will convert to a binary format via inet_aton */
  memset((char *)&remaddr, 0, sizeof(remaddr));
  remaddr.sin_family = AF_INET;
  remaddr.sin_port = htons(port);
  if (inet_aton(host, &remaddr.sin_addr) == 0)
  {
    fprintf(stderr, "inet_aton() failed\n");
    return -1;
  }
  return 0;
}

//----------------------------------------------------------------------//
//- Private methods                                                    -//
//----------------------------------------------------------------------//

/*!
 *	Callback definition called each time a new log is received.
 *
 *	\param[in]	pHandle									Valid handle on the sbgECom instance that has called this callback.
 *	\param[in]	msgClass								Class of the message we have received
 *	\param[in]	msg										Message ID of the log received.
 *	\param[in]	pLogData								Contains the received log data as an union.
 *	\param[in]	pUserArg								Optional user supplied argument.
 *	\return												SBG_NO_ERROR if the received log has been used successfully.
 */
SbgErrorCode onLogReceived(SbgEComHandle *pHandle, SbgEComClass msgClass, SbgEComMsgId msg, const SbgBinaryLogData *pLogData, void *pUserArg)
{
  assert(pLogData);

  SBG_UNUSED_PARAMETER(pHandle);
  SBG_UNUSED_PARAMETER(pUserArg);

  if (msgClass == SBG_ECOM_CLASS_LOG_ECOM_0)
  {
    //
    // Handle separately each received data according to the log ID
    //
    switch (msg)
    {
    case SBG_ECOM_LOG_UTC_TIME:
      message.year = pLogData->utcData.year;
      message.month = pLogData->utcData.month;
      message.day = pLogData->utcData.day;
      message.hour = pLogData->utcData.hour;
      message.minute = pLogData->utcData.minute;
      message.second = pLogData->utcData.second;
      message.nanosecond = pLogData->utcData.nanoSecond;

      // Serialize the struct into a byte buffer
      uint8_t buffer[sizeof(struct Message_s)];
      memcpy(buffer, &message, sizeof(struct Message_s));

      // Send the byte buffer using sendto
      ssize_t sentBytes = sendto(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remaddr, slen);
      if (sentBytes == -1)
      {
        perror("sendto");
        return -1;
      }
      
      break;
    
    case SBG_ECOM_LOG_IMU_DATA:
      message.accel[0] = pLogData->imuData.accelerometers[0];
      message.accel[1] = pLogData->imuData.accelerometers[1];
      message.accel[2] = pLogData->imuData.accelerometers[2];
      message.gyro[0] = pLogData->imuData.gyroscopes[0];
      message.gyro[1] = pLogData->imuData.gyroscopes[1];
      message.gyro[2] = pLogData->imuData.gyroscopes[2];
      message.temperature = pLogData->imuData.temperature;
      message.deltaVelocity[0] = pLogData->imuData.deltaVelocity[0];
      message.deltaVelocity[1] = pLogData->imuData.deltaVelocity[1];
      message.deltaVelocity[2] = pLogData->imuData.deltaVelocity[2];
      message.deltaAngle[0] = pLogData->imuData.deltaAngle[0];
      message.deltaAngle[1] = pLogData->imuData.deltaAngle[1];
      message.deltaAngle[2] = pLogData->imuData.deltaAngle[2];
      break;

    case SBG_ECOM_LOG_GPS1_POS:
      message.altitude = pLogData->gpsPosData.altitude;
      message.latitude = pLogData->gpsPosData.latitude;
      message.longitude = pLogData->gpsPosData.longitude;
      break;

    default:
      break;
    }
  }

  return SBG_NO_ERROR;
}

/*!
 * Get and print product info.
 *
 * \param[in]	pECom					SbgECom instance.
 * \return								SBG_NO_ERROR if successful.
 */
static SbgErrorCode getAndPrintProductInfo(SbgEComHandle *pECom)
{
  SbgErrorCode errorCode;
  SbgEComDeviceInfo deviceInfo;

  assert(pECom);

  //
  // Get device inforamtions
  //
  errorCode = sbgEComCmdGetInfo(pECom, &deviceInfo);

  //
  // Display device information if no error
  //
  if (errorCode == SBG_NO_ERROR)
  {
    char calibVersionStr[32];
    char hwRevisionStr[32];
    char fmwVersionStr[32];

    sbgVersionToStringEncoded(deviceInfo.calibationRev, calibVersionStr, sizeof(calibVersionStr));
    sbgVersionToStringEncoded(deviceInfo.hardwareRev, hwRevisionStr, sizeof(hwRevisionStr));
    sbgVersionToStringEncoded(deviceInfo.firmwareRev, fmwVersionStr, sizeof(fmwVersionStr));

    printf("      Serial Number: %0.9" PRIu32 "\n", deviceInfo.serialNumber);
    printf("       Product Code: %s\n", deviceInfo.productCode);
    printf("  Hardware Revision: %s\n", hwRevisionStr);
    printf("   Firmware Version: %s\n", fmwVersionStr);
    printf("     Calib. Version: %s\n", calibVersionStr);
    printf("\n");
  }
  else
  {
    SBG_LOG_WARNING(errorCode, "Unable to retrieve device information");
  }

  return errorCode;
}

/*!
 * Execute the ellipseMinimal example given an opened and valid interface.
 *
 * \param[in]	pInterface							Interface used to communicate with the device.
 * \return											SBG_NO_ERROR if successful.
 */
static SbgErrorCode SBG_RunProcess(SbgInterface *pInterface)
{
  SbgErrorCode errorCode = SBG_NO_ERROR;
  SbgEComHandle comHandle;

  assert(pInterface);

  //
  // Create the sbgECom library and associate it with the created interfaces
  //
  errorCode = sbgEComInit(&comHandle, pInterface);

  //
  // Test that the sbgECom has been initialized
  //
  if (errorCode == SBG_NO_ERROR)
  {
    printf("sbgECom version %s\n\n", SBG_E_COM_VERSION_STR);

    //
    // Query and display produce info, don't stop if there is an error
    //
    getAndPrintProductInfo(&comHandle);

    
    // Showcase how to configure some output logs to 25 Hz, don't stop if there is an error
    
    errorCode = sbgEComCmdOutputSetConf(&comHandle, SBG_ECOM_OUTPUT_PORT_A, SBG_ECOM_CLASS_LOG_ECOM_0, SBG_ECOM_LOG_UTC_TIME, SBG_ECOM_OUTPUT_MODE_DIV_40);

    if (errorCode != SBG_NO_ERROR)
    {
      SBG_LOG_WARNING(errorCode, "Unable to configure SBG_ECOM_LOG_IMU_DATA log");
    }

    errorCode = sbgEComCmdOutputSetConf(&comHandle, SBG_ECOM_OUTPUT_PORT_A, SBG_ECOM_CLASS_LOG_ECOM_0, SBG_ECOM_LOG_IMU_DATA, SBG_ECOM_OUTPUT_MODE_DIV_40);

    if (errorCode != SBG_NO_ERROR)
    {
      SBG_LOG_WARNING(errorCode, "Unable to configure SBG_ECOM_LOG_IMU_DATA log");
    }

    errorCode = sbgEComCmdOutputSetConf(&comHandle, SBG_ECOM_OUTPUT_PORT_A, SBG_ECOM_CLASS_LOG_ECOM_0, SBG_ECOM_LOG_GPS1_POS, SBG_ECOM_OUTPUT_MODE_DIV_200);

    if (errorCode != SBG_NO_ERROR)
    {
      SBG_LOG_WARNING(errorCode, "Unable to configure SBG_ECOM_LOG_GPS2_POS log");
    }

    //
    // Define callbacks for received data and display header
    //
    sbgEComSetReceiveLogCallback(&comHandle, onLogReceived, NULL);

    //
    // Loop until the user exist
    //
    while (1)
    {
      //
      // Try to read a frame
      //
      errorCode = sbgEComHandle(&comHandle);

      //
      // Test if we have to release some CPU (no frame received)
      //
      if (errorCode == SBG_NOT_READY)
      {
        //
        // Release CPU
        //
        sbgSleep(1);
      }
      else
      {
        SBG_LOG_ERROR(errorCode, "Unable to process incoming sbgECom logs");
      }
    }

    //
    // Close the sbgEcom library
    //
    sbgEComClose(&comHandle);
  }
  else
  {
    SBG_LOG_ERROR(errorCode, "Unable to initialize the sbgECom library");
  }

  return errorCode;
}

static SbgErrorCode sbg_init(void)
{
  SbgErrorCode errorCode = SBG_NO_ERROR;
  SbgInterface sbgInterface;
  int exitCode;
  //
  // Create a serial interface to communicate with the PULSE
  //
  errorCode = sbgInterfaceSerialCreate(&sbgInterface, SBG_DEVICENAME, SBG_BAUDRATE);

  if (errorCode == SBG_NO_ERROR)
  {
    errorCode = SBG_RunProcess(&sbgInterface);

    if (errorCode == SBG_NO_ERROR)
    {
      exitCode = EXIT_SUCCESS;
    }
    else
    {
      exitCode = EXIT_FAILURE;
    }

    sbgInterfaceDestroy(&sbgInterface);
  }
  else
  {
    SBG_LOG_ERROR(errorCode, "unable to open serial interface");
    exitCode = EXIT_FAILURE;
  }

  return exitCode;
}

int main(void)
{
  udp_init();
  sbg_init();

  const char *msg = "test!";

  /* now let's send the messages */
  for (;;)
  {
    printf("Sending datagram '%s' to '%s' on port %d\n", msg, host, port);
    if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *)&remaddr, slen) == -1)
    {
      perror("sendto");
      return -1;
    }
  }

  close(fd);
  return 0; /* ok */
}
