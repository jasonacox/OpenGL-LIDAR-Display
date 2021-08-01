/*
 * Slamtec LIDAR Display for OpenGL
 * 
 * This code reads point cloud data from a Slamtec RPLIDAR Device and
 * displays the output on an OpenGL display in realtime.
 * 
 * Author: Jason A. Cox - https://github.com/jasonacox/OpenGL-LIDAR-Display
 * Date: 2021 July 31
 * 
 * Requires: 
 *      OpenGL (framework)
 *      GLUT (framework)
 *      Slamtec RPLIDAR SDK:  https://github.com/Slamtec/rplidar_sdk 
 */

// Turn off GL Deprecation warnings
#define GL_SILENCE_DEPRECATION

// Settings 
#define PORT "/dev/tty.SLAB_USBtoUART"   // macOS UART for RPLIDAR
#define SCALE 0.1   // Distance scale for screen
#define SCREENX 800 // Size of Display window
#define SCREENY 500 
#define STARTX 100  // Coordinates to place window
#define STARTY 100

// Import Headers
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#include <OpenGL/gl.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "include/rplidar.h" //RPLIDAR sdk
#include <signal.h>

// Function to flag ctrl-c
bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}

/*
 * OPENGL HELPERS
 */

// Set up some colors
class color {
public:
    float r;
    float g;
    float b;
    color(float rV=0.0, float gV=0.0, float bV=0.0);
};

color::color( float rV, float gV, float bV)
{
    r = rV;
    g = gV;
    b = bV;
}
color RED(1.0,0.0,0.0);
color GREEN(0.0,1.0,0.0);
color BLUE(0.0,0.0,1.0);

void point_polar(float theta, float r, int offsetx, int offsety, float size, color c)
{
    int x = r * cos(theta) + offsetx;
    int y = r * sin(theta) + offsety;
    glPointSize(size);
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_POINTS);
    glVertex2f(x,y);
    glEnd();
    // printf("point_polar: th=%f r=%f, %d, %d\n", theta, r, x, y);   
}

void line(int x1, int y1, int x2, int y2, float width, color c)
{
    glColor3f(c.r, c.g, c.b);
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2i(x1,y1);
    glVertex2i(x2,y2);
    glEnd();
}

/*
 * LIDAR HELPERS - from Slamtec LIDAR SDK example
 */

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace rp::standalone::rplidar;

bool checkRPLIDARHealth(RPlidarDriver * drv)
{
    u_result     op_result;
    rplidar_response_device_health_t healthinfo;


    op_result = drv->getHealth(healthinfo);
    if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        printf("RPLidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, rplidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want rplidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

/*
 * MAIN FUNCTIONS
 */

// GLOBALS 
RPlidarDriver * drv;
const char * opt_com_path = NULL;
_u32         baudrateArray[2] = {115200, 256000};
_u32         opt_com_baudrate = 0;
u_result     op_result;
bool useArgcBaudrate = false;

// RENDER - Pull data from LIDAR and render to display
void renderScreen(void){
    glClear (GL_COLOR_BUFFER_BIT);
    float theta = 0.0;
    float dist = 0.1;
    int quality = 0;

    // Fetch data from LIDAR
    rplidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);

    op_result = drv->grabScanDataHq(nodes, count);
    if (IS_OK(op_result)) {
        drv->ascendScanData(nodes, count);
        for (int pos = 0; pos < (int)count ; ++pos) {
            theta = (360-((nodes[pos].angle_z_q14 * 90.f / (1 << 14)))/360)*2*M_PI;
            dist = (nodes[pos].dist_mm_q2/4.0f)*SCALE;
            quality = nodes[pos].quality;
            // Display
            if(quality == 0) {
                point_polar(theta, dist, SCREENX/2, SCREENY/2, 7, RED);
            }
            else {
                point_polar(theta, dist, SCREENX/2, SCREENY/2, 3, BLUE);
            }
            // printf("\r > Data: %f %f %d      ",theta,dist,quality);
        }
    }
    // Render
    glFlush();

    // Intercept request to stop and end gracefully
    if (ctrl_c_pressed){ 
        printf(" Received - Stopping - Shutting down LIDAR\n");
        drv->stop();
        drv->stopMotor();
        RPlidarDriver::DisposeDriver(drv);
        drv = NULL;
        exit(0);
    }
}

// MAIN
int main(int argc, char** argv) {

    // Trap Ctrl-C
    signal(SIGINT, ctrlc);

    // Initialize OpenGL and display
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE |GLUT_RGB);
    glutInitWindowSize (SCREENX,SCREENY);
    glutInitWindowPosition (STARTX, STARTY);
    glutCreateWindow ("OpenGL LIDAR Display");
    glClearColor (1.0, 1.0, 1.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, SCREENX, 0, SCREENY);
    glClear (GL_COLOR_BUFFER_BIT);

    printf("LIDAR OpenGL Display for Slamtec RPLIDAR Device\n"
           "Using RPLIDAR SDK Version: " RPLIDAR_SDK_VERSION "\n");

    // Optional - read serial port from the command line...
    if (argc>1) opt_com_path = argv[1]; 

    // Optional - read baud rate from the command line if specified...
    if (argc>2)
    {
        opt_com_baudrate = strtoul(argv[2], NULL, 10);
        useArgcBaudrate = true;
    }

    // Use default if not specified
    if (!opt_com_path) {
        opt_com_path = PORT;
    }

    // Create the RPLIDAR SDK driver instance
	drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
    if (!drv) {
        printf("insufficent memory, exit\n");
        exit(-2);
    }
    
    // Grab device info
    rplidar_response_device_info_t devinfo;
    bool connectSuccess = false;
    size_t i;
    if(useArgcBaudrate)
    {
        if(!drv)
            drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
        if (IS_OK(drv->connect(opt_com_path, opt_com_baudrate)))
        {
            op_result = drv->getDeviceInfo(devinfo);

            if (IS_OK(op_result)) 
            {
                connectSuccess = true;
            }
            else
            {
                delete drv;
                drv = NULL;
            }
        }
    }
    else
    {
        size_t baudRateArraySize = (sizeof(baudrateArray))/ (sizeof(baudrateArray[0]));
        for(i = 0; i < baudRateArraySize; ++i)
        {
            if(!drv)
                drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
            if(IS_OK(drv->connect(opt_com_path, baudrateArray[i])))
            {
                op_result = drv->getDeviceInfo(devinfo);

                if (IS_OK(op_result)) 
                {
                    connectSuccess = true;
                    break;
                }
                else
                {
                    delete drv;
                    drv = NULL;
                }
            }
        }
    }
    if (!connectSuccess) {
        fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
            , opt_com_path);
        RPlidarDriver::DisposeDriver(drv);
        drv = NULL;
        return 0;
    }

    // Display RPLIDAR Info
    printf("Connected via %s at %d baud\n", opt_com_path, baudrateArray[i]);
    printf("RPLIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }
    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);

    // Check health of device
    if (checkRPLIDARHealth(drv)) {
        // Device is good - start
        drv->startMotor();

        // Start scanning and render loop
        printf("Launching OpenGL Window\n");
        drv->startScan(0,1);
        glutDisplayFunc(renderScreen);
        glutIdleFunc(renderScreen);
        glutMainLoop();
        
        // End
        drv->stop();
        drv->stopMotor();
        RPlidarDriver::DisposeDriver(drv);
        drv = NULL;
        return 0;
        // done!
    }
    else {
        // Device is not OK - end
        RPlidarDriver::DisposeDriver(drv);
        drv = NULL;
        return 0;
    }
}

