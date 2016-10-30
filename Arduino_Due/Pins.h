#ifndef _PINS_H_
#define _PINS_H_

//--- Defines: -------------------------------------------------------------------
#define   POSITIVE                  1
#define   NEGATIVE                  0

#define   STEPS_PER_SQUARE          7100    
#define   STEPS_MIN                 0
#define   STEPS_MAX                 63900         //(STEPS_PER_SQUARE * 9)
#define   HALF_SQUARE               3550          //STEPS_PER_SQUARE/2
#define   STEPS_EXTRA               500
#define   X_OFFSET                  -1250
#define   Y_OFFSET                  0
#define   X_ZERO_OFFSET             0
#define   Y_ZERO_OFFSET             1000

#define   RED                       0
#define   GREEN                     1
#define   BLUE                      2

#define   WHITE                     1
#define   BLACK                     0

#define   USER                      1
#define   COMPUTER                  2

//--- Pin definitions: -----------------------------------------------------------
#define   servo_pin                 13

#define   X_Limit_Switch            8                                             // The switch pulls this pin low when the magnet head is in the (0, Y) Position
#define   Y_Limit_Switch            9                                             // The switch pulls this pin low when the magnet head is in the (X, 0) Position

#define   Mode_Button               52                                            // 
#define   Mode_Button_Light         53

#define   X_Stepper_Dir             2                                             // Direction control for X axis stepper motor
#define   X_Stepper_Step            3                                             // Step pin for X axis stepper motor
#define   Y_Stepper_Dir             4                                             // Direction control for Y axis stepper motor
#define   Y_Stepper_Step            5                                             // Step pin for Y axis stepper motor

#define   Mag_Sense_0               14                                            // Magnetoresitive Sense Line 0
#define   Mag_Sense_1               15                                            // Magnetoresitive Sense Line 1
#define   Mag_Sense_2               16                                            // Magnetoresitive Sense Line 2
#define   Mag_Sense_3               17                                            // Magnetoresitive Sense Line 3
#define   Mag_Sense_4               18                                            // Magnetoresitive Sense Line 4
#define   Mag_Sense_5               19                                            // Magnetoresitive Sense Line 5
#define   Mag_Sense_6               20                                            // Magnetoresitive Sense Line 6
#define   Mag_Sense_7               21                                            // Magnetoresitive Sense Line 7

#define   Darlington_Pin_0          22                                            // Darlington 0
#define   Darlington_Pin_1          23                                            // Darlington 1
#define   Darlington_Pin_2          24                                            // Darlington 2
#define   Darlington_Pin_3          25                                            // Darlington 3
#define   Darlington_Pin_4          26                                            // Darlington 4
#define   Darlington_Pin_5          27                                            // Darlington 5
#define   Darlington_Pin_6          28                                            // Darlington 6
#define   Darlington_Pin_7          29                                            // Darlington 7

#define   R_Channel_0               30                                            // Red Channel 0
#define   R_Channel_1               31                                            // Red Channel 1
#define   R_Channel_2               32                                            // Red Channel 2
#define   R_Channel_3               33                                            // Red Channel 3
#define   R_Channel_4               34                                            // Red Channel 4
#define   R_Channel_5               35                                            // Red Channel 5
#define   R_Channel_6               36                                            // Red Channel 6
#define   R_Channel_7               37                                            // Red Channel 7

#define   G_Channel_0               38                                            // Green Channel 0
#define   G_Channel_1               39                                            // Green Channel 1
#define   G_Channel_2               40                                            // Green Channel 2
#define   G_Channel_3               41                                            // Green Channel 3
#define   G_Channel_4               42                                            // Green Channel 4
#define   G_Channel_5               43                                            // Green Channel 5
#define   G_Channel_6               44                                            // Green Channel 6
#define   G_Channel_7               45                                            // Green Channel 7

#define   B_Channel_0               46                                            // Blue Channel 0
#define   B_Channel_1               47                                            // Blue Channel 1
#define   B_Channel_2               48                                            // Blue Channel 2
#define   B_Channel_3               49                                            // Blue Channel 3
#define   B_Channel_4               50                                            // Blue Channel 4
#define   B_Channel_5               51                                            // Blue Channel 5
#define   B_Channel_6               52                                            // Blue Channel 6
#define   B_Channel_7               53                                            // Blue Channel 7

//--- Routines -------------------------------------------------------------------

//--- Classes --------------------------------------------------------------------


#endif 






