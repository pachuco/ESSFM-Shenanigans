#include <windows.h>

#define WHOOPS Beep(1000, 500); SleepEx(5000, 1);

// only data 1, 2 and 3 are written to port 0x03
int TEST_ESS_doesPort3WriteOnly012(Song* song) {
    BYTE data[] = {0x00, 0x01, 0x02};
    
    for (int i=0; i < song->dataSize; i++) {
        if (song->data[i].port == 0x03) {
            BOOL success = FALSE;
            for (int j=0; j < sizeof(data); j++) {
                if (song->data[i].data == data[j]) success = TRUE;
            }
            if (!success) {
                printf("!%d!\n", i);
                WHOOPS
                return i;
            }
        }
    }
    return -1;
}

// the general succession of writes is as shown below, except for init and stop
// however, the driver may hiccup and ommit some of the writes
int TEST_ESS_isPortProgression231(Song* song) {
    BYTE prog[] = {0x02, 0x03, 0x01};
    
    for (int i=0; i < song->dataSize; i++) {
        if (song->data[i].port != prog[i%sizeof(prog)]) {
            printf("!%d!\n", i);
            WHOOPS
            return i;
        }
    }
    return -1;
}