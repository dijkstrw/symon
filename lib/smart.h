#ifndef _SYMON_LIB_SMART_H
#define _SYMON_LIB_SMART_H

#define MAX_SMART_ATTRIBUTES 30

/* The structures below are what the disks send back verbatim. Byte-alignment
 * is mandatory here.
 */
#pragma pack(1)
struct smart_attribute {
    u_int8_t id;
    u_int16_t flags;
    u_int8_t current;
    u_int8_t worst;
    u_int8_t raw[6];
    u_int8_t res;
};

/* smart_values structure should be 512 = one disk block exactly */
struct smart_values {
    u_int16_t rev;
    struct smart_attribute attributes[MAX_SMART_ATTRIBUTES];
    u_int8_t offline_status;
    u_int8_t test_status;
    u_int16_t offline_time;
    u_int8_t vendor1;
    u_int8_t offline_cap;
    u_int16_t smart_cap;
    u_int8_t errlog_capl;
    u_int8_t vendor2;
    u_int8_t stest_ctime;
    u_int8_t etest_ctime;
    u_int8_t ctest_ctime;
    u_int8_t res[11];
    u_int8_t vendor3[125];
    u_int8_t sum;
};
#pragma pack()

#define ATA_ATTRIBUTE_END                          0x00
#define ATA_ATTRIBUTE_READ_ERROR_RATE              0x01
#define ATA_ATTRIBUTE_REALLOCATED_SECTOR_COUNT     0x05
#define ATA_ATTRIBUTE_SPIN_RETRY_COUNT             0x0a
#define ATA_ATTRIBUTE_AIR_FLOW_TEMPERATURE         0xbe
#define ATA_ATTRIBUTE_TEMPERATURE                  0xc2
#define ATA_ATTRIBUTE_REALLOCATION_EVENT_COUNT     0xc4
#define ATA_ATTRIBUTE_CURRENT_PENDING_SECTOR_COUNT 0xc5
#define ATA_ATTRIBUTE_UNCORRECTABLE_SECTOR_COUNT   0xc6
#define ATA_ATTRIBUTE_SOFT_READ_ERROR_RATE         0xc9
#define ATA_ATTRIBUTE_G_SENSE_ERROR_RATE           0xdd
#define ATA_ATTRIBUTE_TEMPERATURE2                 0xe7
#define ATA_ATTRIBUTE_FREE_FALL_PROTECTION         0xfe

struct smart_report {
    int read_error_rate;
    int reallocated_sectors;
    int spin_retries;
    int air_flow_temp;
    int temperature;
    int reallocations;
    int current_pending;
    int uncorrectables;
    int soft_read_error_rate;
    int g_sense_error_rate;
    int temperature2;
    int free_fall_protection;
};

extern void smart_parse(struct smart_values *ds, struct smart_report *sr);

#endif /* _SYMON_LIB_SMART_H */
