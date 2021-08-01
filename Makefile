# MAKEFILE
#
# This makefile builds an OpenGL based LIDAR display for the Slamtec RPLIDAR 
# device.  It requires the Slamtec rplidar sdk libraries and headers which
# can be built with the build.sh script.
#
# Jason Cox, @jasonacox
#   https://github.com/jasonacox/OpenGL-LIDAR-Display 
#
# 'make'        build executable file 'lidar'
# 'make clean'  removes all .o and executable files
#
# Manual: 
# 	gcc lidar.cpp librplidar_sdk.a -o lidar -framework OpenGL -framework GLUT -lstdc++ -lpthread

# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g

# define any directories containing header files other than /usr/include
INCLUDES = -I./include -I./include/hal

# define library paths 
LFLAGS = ./lib/librplidar_sdk.a

# define any libraries and frameworks to link into executable:
LIBS = -framework OpenGL -framework GLUT -lstdc++ -lpthread 

# define the C source files
SRCS = lidar.c

# define the C object files 
OBJS = $(SRCS:.c=.o)

# define the executable file 
MAIN = lidar

# make
all:    $(MAIN)
	@echo
	@echo "OpenGL LIDAR Display built - see ./bin/lidar"
	@cp lidar ./bin

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

