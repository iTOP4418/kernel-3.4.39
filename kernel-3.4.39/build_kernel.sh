#!/bin/bash

#base compile
#cp  ./s5p4418_topeet_android_defconfig .config
cp  ./config_for_iTOP4418 .config
make ARCH=arm   CROSS_COMPILE=/media/work/s5p4418/kitkat-s5p4418drone/android/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-      uImage -j4
#make ARCH=arm   CROSS_COMPILE=/media/work/s5p4418/kitkat-s5p4418drone/android/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-      modules	


#install mt6620 moudles to android/device/nexell/drone2/mt6620
#cp ./drivers/misc/mediatek/combo_mt66xx/wmt/*.ko    ../../../device/nexell/drone2/mt6620/  -rf
#cp ./drivers/misc/mediatek/combo_mt66xx/fm/*.ko    ../../../device/nexell/drone2/mt6620/  -rf
#cp ./drivers/net/wireless/combo_mt66xx/mt6620/wlan/*.ko  ../../../device/nexell/drone2/mt6620/  -rf   




