/******************************************/
/* NOTE: B6/C3 is data header signature   */
/*       0xAA/0xBB is data length = total */
/*       bytes - 7, 0xCC is type, 0xDD is */
/*       interrupt to use.                */
/******************************************/

/****************************************************************
 *     kaweth_trigger_code
 ****************************************************************/
static __u8 kaweth_trigger_code[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0xc8, 0x07, 0xa0, 0x00, 0xf0, 0x07, 0x5e, 0x00,
    0x06, 0x00, 0xf0, 0x07, 0x0a, 0x00, 0x08, 0x00,
    0xf0, 0x09, 0x00, 0x00, 0x02, 0x00, 0xe7, 0x07,
    0x36, 0x00, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00,
    0x04, 0x00, 0xe7, 0x07, 0x50, 0xc3, 0x10, 0xc0,
    0xf0, 0x09, 0x0e, 0xc0, 0x00, 0x00, 0xe7, 0x87,
    0x01, 0x00, 0x0e, 0xc0, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0x17, 0x02, 0xc8, 0x07, 0xa0, 0x00,
    0xe7, 0x17, 0x50, 0xc3, 0x10, 0xc0, 0x30, 0xd8,
    0x04, 0x00, 0x30, 0x5c, 0x08, 0x00, 0x04, 0x00,
    0xb0, 0xc0, 0x06, 0x00, 0xc8, 0x05, 0xe7, 0x05,
    0x00, 0xc0, 0xc0, 0xdf, 0x97, 0xcf, 0x49, 0xaf,
    0xc0, 0x07, 0x00, 0x00, 0x60, 0xaf, 0x4a, 0xaf,
    0x00, 0x0c, 0x0c, 0x00, 0x40, 0xd2, 0x00, 0x1c,
    0x0c, 0x00, 0x40, 0xd2, 0x30, 0x00, 0x08, 0x00,
    0xf0, 0x07, 0x00, 0x00, 0x04, 0x00, 0xf0, 0x07,
    0x86, 0x00, 0x06, 0x00, 0x67, 0xcf, 0x27, 0x0c,
    0x02, 0x00, 0x00, 0x00, 0x27, 0x0c, 0x00, 0x00,
    0x0e, 0xc0, 0x49, 0xaf, 0x64, 0xaf, 0xc0, 0x07,
    0x00, 0x00, 0x4b, 0xaf, 0x4a, 0xaf, 0x5a, 0xcf,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x94, 0x00, 0x05, 0x00,
    0x00, 0x00
};
/****************************************************************
 *     kaweth_trigger_code_fix
 ****************************************************************/
static __u8 kaweth_trigger_code_fix[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0x02, 0x00, 0x06, 0x00, 0x18, 0x00, 0x3e, 0x00,
    0x80, 0x00, 0x98, 0x00, 0xaa, 0x00,
    0x00, 0x00
};

/****************************************************************
 *     kaweth_new_code
 ****************************************************************/
static __u8 kaweth_new_code[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0x9f, 0xcf, 0xde, 0x06, 0xe7, 0x57, 0x00, 0x00,
    0xc4, 0x06, 0x97, 0xc1, 0xe7, 0x67, 0xff, 0x1f,
    0x28, 0xc0, 0xe7, 0x87, 0x00, 0x04, 0x24, 0xc0,
    0xe7, 0x67, 0xff, 0xf9, 0x22, 0xc0, 0x97, 0xcf,
    0xd7, 0x09, 0x00, 0xc0, 0xe7, 0x09, 0xa2, 0xc0,
    0xbe, 0x06, 0x9f, 0xaf, 0x36, 0x00, 0xe7, 0x05,
    0x00, 0xc0, 0xa7, 0xcf, 0xbc, 0x06, 0x97, 0xcf,
    0xe7, 0x57, 0x00, 0x00, 0xb8, 0x06, 0xa7, 0xa1,
    0xb8, 0x06, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0x14, 0x08, 0x0a, 0xc0, 0xe7, 0x57, 0x00, 0x00,
    0xa4, 0xc0, 0xa7, 0xc0, 0x7a, 0x06, 0x9f, 0xaf,
    0x92, 0x07, 0xe7, 0x07, 0x00, 0x00, 0x14, 0x08,
    0xe7, 0x57, 0xff, 0xff, 0xba, 0x06, 0x9f, 0xa0,
    0x38, 0x00, 0xe7, 0x59, 0xba, 0x06, 0xbe, 0x06,
    0x9f, 0xa0, 0x38, 0x00, 0xc8, 0x09, 0xca, 0x06,
    0x08, 0x62, 0x9f, 0xa1, 0x36, 0x08, 0xc0, 0x09,
    0x76, 0x06, 0x00, 0x60, 0xa7, 0xc0, 0x7a, 0x06,
    0x9f, 0xaf, 0xcc, 0x02, 0xe7, 0x57, 0x00, 0x00,
    0xb8, 0x06, 0xa7, 0xc1, 0x7a, 0x06, 0x9f, 0xaf,
    0x04, 0x00, 0xe7, 0x57, 0x00, 0x00, 0x8e, 0x06,
    0x0a, 0xc1, 0xe7, 0x09, 0x20, 0xc0, 0x10, 0x08,
    0xe7, 0xd0, 0x10, 0x08, 0xe7, 0x67, 0x40, 0x00,
    0x10, 0x08, 0x9f, 0xaf, 0x92, 0x0c, 0xc0, 0x09,
    0xd0, 0x06, 0x00, 0x60, 0x05, 0xc4, 0xc0, 0x59,
    0xbe, 0x06, 0x02, 0xc0, 0x9f, 0xaf, 0xec, 0x00,
    0x9f, 0xaf, 0x34, 0x02, 0xe7, 0x57, 0x00, 0x00,
    0xa6, 0x06, 0x9f, 0xa0, 0x7a, 0x02, 0xa7, 0xcf,
    0x7a, 0x06, 0x48, 0x02, 0xe7, 0x09, 0xbe, 0x06,
    0xd0, 0x06, 0xc8, 0x37, 0x04, 0x00, 0x9f, 0xaf,
    0x08, 0x03, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0xce, 0x06, 0x97, 0xc0, 0xd7, 0x09, 0x00, 0xc0,
    0xc1, 0xdf, 0xc8, 0x09, 0xc6, 0x06, 0x08, 0x62,
    0x14, 0xc0, 0x27, 0x04, 0xc6, 0x06, 0x10, 0x94,
    0xf0, 0x07, 0x10, 0x08, 0x02, 0x00, 0xc1, 0x07,
    0x01, 0x00, 0x70, 0x00, 0x04, 0x00, 0xf0, 0x07,
    0x30, 0x01, 0x06, 0x00, 0x50, 0xaf, 0xe7, 0x07,
    0xff, 0xff, 0xd0, 0x06, 0xe7, 0x07, 0x00, 0x00,
    0xce, 0x06, 0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x48, 0x02,
    0xd0, 0x09, 0xc6, 0x06, 0x27, 0x02, 0xc6, 0x06,
    0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf, 0x48, 0x02,
    0xc8, 0x37, 0x04, 0x00, 0x00, 0x0c, 0x0c, 0x00,
    0x00, 0x60, 0x21, 0xc0, 0xc0, 0x37, 0x3e, 0x00,
    0x23, 0xc9, 0xc0, 0x57, 0xb4, 0x05, 0x1b, 0xc8,
    0xc0, 0x17, 0x3f, 0x00, 0xc0, 0x67, 0xc0, 0xff,
    0x30, 0x00, 0x08, 0x00, 0xf0, 0x07, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x02, 0xc0, 0x17, 0x4c, 0x00,
    0x30, 0x00, 0x06, 0x00, 0xf0, 0x07, 0xa0, 0x01,
    0x0a, 0x00, 0x48, 0x02, 0xc1, 0x07, 0x02, 0x00,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x51, 0xaf,
    0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf, 0x9f, 0xaf,
    0x08, 0x03, 0x9f, 0xaf, 0x7a, 0x02, 0x97, 0xcf,
    0x9f, 0xaf, 0x7a, 0x02, 0xc9, 0x37, 0x04, 0x00,
    0xc1, 0xdf, 0xc8, 0x09, 0xa2, 0x06, 0x50, 0x02,
    0x67, 0x02, 0xa2, 0x06, 0xd1, 0x07, 0x00, 0x00,
    0x27, 0xd8, 0xaa, 0x06, 0xc0, 0xdf, 0x9f, 0xaf,
    0xc4, 0x01, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0xd2, 0x06, 0x97, 0xc1, 0xe7, 0x57, 0x01, 0x00,
    0xa8, 0x06, 0x97, 0xc0, 0xc8, 0x09, 0xa0, 0x06,
    0x08, 0x62, 0x97, 0xc0, 0x00, 0x02, 0xc0, 0x17,
    0x0e, 0x00, 0x27, 0x00, 0x34, 0x01, 0x27, 0x0c,
    0x0c, 0x00, 0x36, 0x01, 0xe7, 0x07, 0x50, 0xc3,
    0x12, 0xc0, 0xe7, 0x07, 0xcc, 0x0b, 0x02, 0x00,
    0xe7, 0x07, 0x01, 0x00, 0xa8, 0x06, 0xe7, 0x07,
    0x05, 0x00, 0x90, 0xc0, 0x97, 0xcf, 0xc8, 0x09,
    0xa4, 0x06, 0x08, 0x62, 0x02, 0xc0, 0x10, 0x64,
    0x07, 0xc1, 0xe7, 0x07, 0x00, 0x00, 0x9e, 0x06,
    0xe7, 0x07, 0x72, 0x04, 0x24, 0x00, 0x97, 0xcf,
    0x27, 0x04, 0xa4, 0x06, 0xc8, 0x17, 0x0e, 0x00,
    0x27, 0x02, 0x9e, 0x06, 0xe7, 0x07, 0x80, 0x04,
    0x24, 0x00, 0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0,
    0xc1, 0xdf, 0xe7, 0x57, 0x00, 0x00, 0x90, 0x06,
    0x13, 0xc1, 0x9f, 0xaf, 0x06, 0x02, 0xe7, 0x57,
    0x00, 0x00, 0x9e, 0x06, 0x13, 0xc0, 0xe7, 0x09,
    0x9e, 0x06, 0x30, 0x01, 0xe7, 0x07, 0xf2, 0x05,
    0x32, 0x01, 0xe7, 0x07, 0x10, 0x00, 0x96, 0xc0,
    0xe7, 0x09, 0x9e, 0x06, 0x90, 0x06, 0x04, 0xcf,
    0xe7, 0x57, 0x00, 0x00, 0x9e, 0x06, 0x02, 0xc1,
    0x9f, 0xaf, 0x06, 0x02, 0xe7, 0x05, 0x00, 0xc0,
    0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf,
    0x08, 0x92, 0xe7, 0x57, 0x02, 0x00, 0xaa, 0x06,
    0x02, 0xc3, 0xc8, 0x09, 0xa4, 0x06, 0x27, 0x02,
    0xa6, 0x06, 0x08, 0x62, 0x03, 0xc1, 0xe7, 0x05,
    0x00, 0xc0, 0x97, 0xcf, 0x27, 0x04, 0xa4, 0x06,
    0xe7, 0x05, 0x00, 0xc0, 0xf0, 0x07, 0x40, 0x00,
    0x08, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x02, 0xc0, 0x17, 0x0c, 0x00, 0x30, 0x00,
    0x06, 0x00, 0xf0, 0x07, 0x46, 0x01, 0x0a, 0x00,
    0xc8, 0x17, 0x04, 0x00, 0xc1, 0x07, 0x02, 0x00,
    0x51, 0xaf, 0x97, 0xcf, 0xe7, 0x57, 0x00, 0x00,
    0x96, 0x06, 0x97, 0xc0, 0xc1, 0xdf, 0xc8, 0x09,
    0x96, 0x06, 0x27, 0x04, 0x96, 0x06, 0x27, 0x52,
    0x98, 0x06, 0x03, 0xc1, 0xe7, 0x07, 0x96, 0x06,
    0x98, 0x06, 0xc0, 0xdf, 0x17, 0x02, 0xc8, 0x17,
    0x0e, 0x00, 0x9f, 0xaf, 0xba, 0x03, 0xc8, 0x05,
    0x00, 0x60, 0x03, 0xc0, 0x9f, 0xaf, 0x24, 0x03,
    0x97, 0xcf, 0x9f, 0xaf, 0x08, 0x03, 0x97, 0xcf,
    0x57, 0x02, 0xc9, 0x07, 0xa4, 0x06, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0x08, 0x62, 0x1b, 0xc0,
    0x50, 0x04, 0x11, 0x02, 0xe7, 0x05, 0x00, 0xc0,
    0xc9, 0x05, 0x97, 0xcf, 0x97, 0x02, 0xca, 0x09,
    0xd6, 0x06, 0xf2, 0x17, 0x01, 0x00, 0x04, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x06, 0x00, 0xca, 0x17,
    0x2c, 0x00, 0xf8, 0x77, 0x01, 0x00, 0x0e, 0x00,
    0x06, 0xc0, 0xca, 0xd9, 0xf8, 0x57, 0xff, 0x00,
    0x0e, 0x00, 0x01, 0xc1, 0xca, 0xd9, 0x22, 0x1c,
    0x0c, 0x00, 0xe2, 0x27, 0x00, 0x00, 0xe2, 0x17,
    0x01, 0x00, 0xe2, 0x27, 0x00, 0x00, 0xca, 0x05,
    0x00, 0x0c, 0x0c, 0x00, 0xc0, 0x17, 0x41, 0x00,
    0xc0, 0x67, 0xc0, 0xff, 0x30, 0x00, 0x08, 0x00,
    0x00, 0x02, 0xc0, 0x17, 0x0c, 0x00, 0x30, 0x00,
    0x06, 0x00, 0xf0, 0x07, 0xda, 0x00, 0x0a, 0x00,
    0xf0, 0x07, 0x00, 0x00, 0x04, 0x00, 0x00, 0x0c,
    0x08, 0x00, 0x40, 0xd1, 0x01, 0x00, 0xc0, 0x19,
    0xce, 0x06, 0xc0, 0x59, 0xc2, 0x06, 0x04, 0xc9,
    0x49, 0xaf, 0x9f, 0xaf, 0xec, 0x00, 0x4a, 0xaf,
    0x67, 0x10, 0xce, 0x06, 0xc8, 0x17, 0x04, 0x00,
    0xc1, 0x07, 0x01, 0x00, 0xd7, 0x09, 0x00, 0xc0,
    0xc1, 0xdf, 0x50, 0xaf, 0xe7, 0x05, 0x00, 0xc0,
    0x97, 0xcf, 0xc0, 0x07, 0x01, 0x00, 0xc1, 0x09,
    0xac, 0x06, 0xc1, 0x77, 0x01, 0x00, 0x97, 0xc1,
    0xd8, 0x77, 0x01, 0x00, 0x12, 0xc0, 0xc9, 0x07,
    0x6a, 0x06, 0x9f, 0xaf, 0x08, 0x04, 0x04, 0xc1,
    0xc1, 0x77, 0x08, 0x00, 0x13, 0xc0, 0x97, 0xcf,
    0xc1, 0x77, 0x02, 0x00, 0x97, 0xc1, 0xc1, 0x77,
    0x10, 0x00, 0x0c, 0xc0, 0x9f, 0xaf, 0x2c, 0x04,
    0x97, 0xcf, 0xc1, 0x77, 0x04, 0x00, 0x06, 0xc0,
    0xc9, 0x07, 0x70, 0x06, 0x9f, 0xaf, 0x08, 0x04,
    0x97, 0xc0, 0x00, 0xcf, 0x00, 0x90, 0x97, 0xcf,
    0x50, 0x54, 0x97, 0xc1, 0x70, 0x5c, 0x02, 0x00,
    0x02, 0x00, 0x97, 0xc1, 0x70, 0x5c, 0x04, 0x00,
    0x04, 0x00, 0x97, 0xcf, 0x80, 0x01, 0xc0, 0x00,
    0x60, 0x00, 0x30, 0x00, 0x18, 0x00, 0x0c, 0x00,
    0x06, 0x00, 0x00, 0x00, 0xcb, 0x09, 0xb2, 0x06,
    0xcc, 0x09, 0xb4, 0x06, 0x0b, 0x53, 0x11, 0xc0,
    0xc9, 0x02, 0xca, 0x07, 0x1c, 0x04, 0x9f, 0xaf,
    0x08, 0x04, 0x97, 0xc0, 0x0a, 0xc8, 0x82, 0x08,
    0x0a, 0xcf, 0x82, 0x08, 0x9f, 0xaf, 0x08, 0x04,
    0x97, 0xc0, 0x05, 0xc2, 0x89, 0x30, 0x82, 0x60,
    0x78, 0xc1, 0x00, 0x90, 0x97, 0xcf, 0x89, 0x10,
    0x09, 0x53, 0x79, 0xc2, 0x89, 0x30, 0x82, 0x08,
    0x7a, 0xcf, 0xc0, 0xdf, 0x97, 0xcf, 0xc0, 0xdf,
    0x97, 0xcf, 0xe7, 0x09, 0x96, 0xc0, 0x92, 0x06,
    0xe7, 0x09, 0x98, 0xc0, 0x94, 0x06, 0x0f, 0xcf,
    0xe7, 0x09, 0x96, 0xc0, 0x92, 0x06, 0xe7, 0x09,
    0x98, 0xc0, 0x94, 0x06, 0xe7, 0x09, 0x9e, 0x06,
    0x30, 0x01, 0xe7, 0x07, 0xf2, 0x05, 0x32, 0x01,
    0xe7, 0x07, 0x10, 0x00, 0x96, 0xc0, 0xd7, 0x09,
    0x00, 0xc0, 0x17, 0x02, 0xc8, 0x09, 0x90, 0x06,
    0xc8, 0x37, 0x0e, 0x00, 0xe7, 0x77, 0x2a, 0x00,
    0x92, 0x06, 0x30, 0xc0, 0x97, 0x02, 0xca, 0x09,
    0xd6, 0x06, 0xe7, 0x77, 0x20, 0x00, 0x92, 0x06,
    0x0e, 0xc0, 0xf2, 0x17, 0x01, 0x00, 0x10, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x12, 0x00, 0xe7, 0x77,
    0x0a, 0x00, 0x92, 0x06, 0xca, 0x05, 0x1e, 0xc0,
    0x97, 0x02, 0xca, 0x09, 0xd6, 0x06, 0xf2, 0x17,
    0x01, 0x00, 0x0c, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x0e, 0x00, 0xe7, 0x77, 0x02, 0x00, 0x92, 0x06,
    0x07, 0xc0, 0xf2, 0x17, 0x01, 0x00, 0x44, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x46, 0x00, 0x06, 0xcf,
    0xf2, 0x17, 0x01, 0x00, 0x60, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x62, 0x00, 0xca, 0x05, 0x9f, 0xaf,
    0x08, 0x03, 0x0f, 0xcf, 0x57, 0x02, 0x09, 0x02,
    0xf1, 0x09, 0x94, 0x06, 0x0c, 0x00, 0xf1, 0xda,
    0x0c, 0x00, 0xc8, 0x09, 0x98, 0x06, 0x50, 0x02,
    0x67, 0x02, 0x98, 0x06, 0xd1, 0x07, 0x00, 0x00,
    0xc9, 0x05, 0xe7, 0x09, 0x9e, 0x06, 0x90, 0x06,
    0xe7, 0x57, 0x00, 0x00, 0x90, 0x06, 0x02, 0xc0,
    0x9f, 0xaf, 0x06, 0x02, 0xc8, 0x05, 0xe7, 0x05,
    0x00, 0xc0, 0xc0, 0xdf, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0x17, 0x00, 0x17, 0x02, 0x97, 0x02,
    0xc0, 0x09, 0x92, 0xc0, 0xe7, 0x07, 0x04, 0x00,
    0x90, 0xc0, 0xca, 0x09, 0xd6, 0x06, 0xe7, 0x07,
    0x00, 0x00, 0xa8, 0x06, 0xe7, 0x07, 0x6a, 0x04,
    0x02, 0x00, 0xc0, 0x77, 0x02, 0x00, 0x08, 0xc0,
    0xf2, 0x17, 0x01, 0x00, 0x50, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x52, 0x00, 0x9f, 0xcf, 0x24, 0x06,
    0xc0, 0x77, 0x10, 0x00, 0x06, 0xc0, 0xf2, 0x17,
    0x01, 0x00, 0x58, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x5a, 0x00, 0xc0, 0x77, 0x80, 0x00, 0x06, 0xc0,
    0xf2, 0x17, 0x01, 0x00, 0x70, 0x00, 0xf2, 0x27,
    0x00, 0x00, 0x72, 0x00, 0xc0, 0x77, 0x08, 0x00,
    0x1d, 0xc1, 0xf2, 0x17, 0x01, 0x00, 0x08, 0x00,
    0xf2, 0x27, 0x00, 0x00, 0x0a, 0x00, 0xc0, 0x77,
    0x00, 0x02, 0x06, 0xc0, 0xf2, 0x17, 0x01, 0x00,
    0x64, 0x00, 0xf2, 0x27, 0x00, 0x00, 0x66, 0x00,
    0xc0, 0x77, 0x40, 0x00, 0x06, 0xc0, 0xf2, 0x17,
    0x01, 0x00, 0x5c, 0x00, 0xf2, 0x27, 0x00, 0x00,
    0x5e, 0x00, 0xc0, 0x77, 0x01, 0x00, 0x01, 0xc0,
    0x1b, 0xcf, 0x1a, 0xcf, 0xf2, 0x17, 0x01, 0x00,
    0x00, 0x00, 0xf2, 0x27, 0x00, 0x00, 0x02, 0x00,
    0xc8, 0x09, 0x34, 0x01, 0xca, 0x17, 0x14, 0x00,
    0xd8, 0x77, 0x01, 0x00, 0x05, 0xc0, 0xca, 0xd9,
    0xd8, 0x57, 0xff, 0x00, 0x01, 0xc0, 0xca, 0xd9,
    0xe2, 0x19, 0x94, 0xc0, 0xe2, 0x27, 0x00, 0x00,
    0xe2, 0x17, 0x01, 0x00, 0xe2, 0x27, 0x00, 0x00,
    0x9f, 0xaf, 0x40, 0x06, 0x9f, 0xaf, 0xc4, 0x01,
    0xe7, 0x57, 0x00, 0x00, 0xd2, 0x06, 0x9f, 0xa1,
    0x0e, 0x0a, 0xca, 0x05, 0xc8, 0x05, 0xc0, 0x05,
    0xe7, 0x05, 0x00, 0xc0, 0xc0, 0xdf, 0x97, 0xcf,
    0xc8, 0x09, 0xa0, 0x06, 0x08, 0x62, 0x97, 0xc0,
    0x27, 0x04, 0xa0, 0x06, 0x27, 0x52, 0xa2, 0x06,
    0x03, 0xc1, 0xe7, 0x07, 0xa0, 0x06, 0xa2, 0x06,
    0x9f, 0xaf, 0x08, 0x03, 0xe7, 0x57, 0x00, 0x00,
    0xaa, 0x06, 0x02, 0xc0, 0x27, 0xda, 0xaa, 0x06,
    0x97, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0xfb, 0x13, 0xe7, 0x57,
    0x00, 0x80, 0xb2, 0x00, 0x06, 0xc2, 0xe7, 0x07,
    0xee, 0x0b, 0x12, 0x00, 0xe7, 0x07, 0x34, 0x0c,
    0xb2, 0x00, 0xe7, 0x07, 0xc6, 0x07, 0xf2, 0x02,
    0xc8, 0x09, 0xb4, 0x00, 0xf8, 0x07, 0x02, 0x00,
    0x0d, 0x00, 0xd7, 0x09, 0x0e, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0x0e, 0xc0, 0xc8, 0x09, 0xde, 0x00,
    0xc8, 0x17, 0x09, 0x00, 0xc9, 0x07, 0xda, 0x06,
    0xc0, 0x07, 0x04, 0x00, 0x68, 0x0a, 0x00, 0xda,
    0x7d, 0xc1, 0xe7, 0x09, 0xc0, 0x00, 0x7c, 0x06,
    0xe7, 0x09, 0xbe, 0x00, 0x78, 0x06, 0xe7, 0x09,
    0x10, 0x00, 0xbc, 0x06, 0xc8, 0x07, 0xd6, 0x07,
    0x9f, 0xaf, 0xae, 0x07, 0x9f, 0xaf, 0x00, 0x0a,
    0xc8, 0x09, 0xde, 0x00, 0x00, 0x0e, 0x0f, 0x00,
    0x41, 0x90, 0x9f, 0xde, 0x06, 0x00, 0x44, 0xaf,
    0x27, 0x00, 0xb2, 0x06, 0x27, 0x00, 0xb4, 0x06,
    0x27, 0x00, 0xb6, 0x06, 0xc0, 0x07, 0x74, 0x00,
    0x44, 0xaf, 0x27, 0x00, 0xd6, 0x06, 0x08, 0x00,
    0x00, 0x90, 0xc1, 0x07, 0x3a, 0x00, 0x20, 0x00,
    0x01, 0xda, 0x7d, 0xc1, 0x9f, 0xaf, 0xba, 0x09,
    0xc0, 0x07, 0x44, 0x00, 0x48, 0xaf, 0x27, 0x00,
    0x7a, 0x06, 0x9f, 0xaf, 0x96, 0x0a, 0xe7, 0x07,
    0x01, 0x00, 0xc0, 0x06, 0xe7, 0x05, 0x0e, 0xc0,
    0x97, 0xcf, 0x49, 0xaf, 0xe7, 0x87, 0x43, 0x00,
    0x0e, 0xc0, 0xe7, 0x07, 0xff, 0xff, 0xbe, 0x06,
    0x9f, 0xaf, 0xae, 0x0a, 0xc0, 0x07, 0x01, 0x00,
    0x60, 0xaf, 0x4a, 0xaf, 0x97, 0xcf, 0x00, 0x08,
    0x09, 0x08, 0x11, 0x08, 0x00, 0xda, 0x7c, 0xc1,
    0x97, 0xcf, 0x67, 0x04, 0xcc, 0x02, 0xc0, 0xdf,
    0x51, 0x94, 0xb1, 0xaf, 0x06, 0x00, 0xc1, 0xdf,
    0xc9, 0x09, 0xcc, 0x02, 0x49, 0x62, 0x75, 0xc1,
    0xc0, 0xdf, 0xa7, 0xcf, 0xd6, 0x02, 0x0e, 0x00,
    0x24, 0x00, 0x80, 0x04, 0x22, 0x00, 0x4e, 0x05,
    0xd0, 0x00, 0x0e, 0x0a, 0xaa, 0x00, 0x30, 0x08,
    0xbe, 0x00, 0x4a, 0x0a, 0x10, 0x00, 0x20, 0x00,
    0x04, 0x00, 0x6e, 0x04, 0x02, 0x00, 0x6a, 0x04,
    0x06, 0x00, 0x00, 0x00, 0x24, 0xc0, 0x04, 0x04,
    0x28, 0xc0, 0xfe, 0xfb, 0x1e, 0xc0, 0x00, 0x04,
    0x22, 0xc0, 0xff, 0xf4, 0xc0, 0x00, 0x90, 0x09,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x56, 0x08,
    0x60, 0x08, 0xd0, 0x08, 0xda, 0x08, 0x00, 0x09,
    0x04, 0x09, 0x08, 0x09, 0x32, 0x09, 0x42, 0x09,
    0x50, 0x09, 0x52, 0x09, 0x5a, 0x09, 0x5a, 0x09,
    0x27, 0x02, 0xca, 0x06, 0x97, 0xcf, 0xe7, 0x07,
    0x00, 0x00, 0xca, 0x06, 0x0a, 0x0e, 0x01, 0x00,
    0xca, 0x57, 0x0e, 0x00, 0x9f, 0xc3, 0x5a, 0x09,
    0xca, 0x37, 0x00, 0x00, 0x9f, 0xc2, 0x5a, 0x09,
    0x0a, 0xd2, 0xb2, 0xcf, 0x16, 0x08, 0xc8, 0x09,
    0xde, 0x00, 0x07, 0x06, 0x9f, 0xcf, 0x6c, 0x09,
    0x17, 0x02, 0xc8, 0x09, 0xde, 0x00, 0x00, 0x0e,
    0x0f, 0x00, 0x41, 0x90, 0x9f, 0xde, 0x06, 0x00,
    0xc8, 0x05, 0x30, 0x50, 0x06, 0x00, 0x9f, 0xc8,
    0x5a, 0x09, 0x27, 0x0c, 0x02, 0x00, 0xb0, 0x06,
    0xc0, 0x09, 0xb2, 0x06, 0x27, 0x00, 0xb4, 0x06,
    0xe7, 0x07, 0x00, 0x00, 0xae, 0x06, 0x27, 0x00,
    0x80, 0x06, 0x00, 0x1c, 0x06, 0x00, 0x27, 0x00,
    0xb6, 0x06, 0x41, 0x90, 0x67, 0x50, 0xb0, 0x06,
    0x0d, 0xc0, 0x67, 0x00, 0x7e, 0x06, 0x27, 0x0c,
    0x06, 0x00, 0x82, 0x06, 0xe7, 0x07, 0xbc, 0x08,
    0x84, 0x06, 0xc8, 0x07, 0x7e, 0x06, 0x41, 0x90,
    0x51, 0xaf, 0x97, 0xcf, 0x9f, 0xaf, 0x48, 0x0c,
    0xe7, 0x09, 0xb6, 0x06, 0xb4, 0x06, 0xe7, 0x09,
    0xb0, 0x06, 0xae, 0x06, 0x59, 0xaf, 0x97, 0xcf,
    0x27, 0x0c, 0x02, 0x00, 0xac, 0x06, 0x59, 0xaf,
    0x97, 0xcf, 0x09, 0x0c, 0x02, 0x00, 0x09, 0xda,
    0x49, 0xd2, 0xc9, 0x19, 0xd6, 0x06, 0xc8, 0x07,
    0x7e, 0x06, 0xe0, 0x07, 0x00, 0x00, 0x60, 0x02,
    0xe0, 0x07, 0x04, 0x00, 0xd0, 0x07, 0xcc, 0x08,
    0x48, 0xdb, 0x41, 0x90, 0x50, 0xaf, 0x97, 0xcf,
    0x59, 0xaf, 0x97, 0xcf, 0x59, 0xaf, 0x97, 0xcf,
    0xf0, 0x57, 0x06, 0x00, 0x06, 0x00, 0x25, 0xc1,
    0xe7, 0x07, 0x70, 0x06, 0x80, 0x06, 0x41, 0x90,
    0x67, 0x00, 0x7e, 0x06, 0x27, 0x0c, 0x06, 0x00,
    0x82, 0x06, 0xe7, 0x07, 0x8c, 0x09, 0x84, 0x06,
    0xc8, 0x07, 0x7e, 0x06, 0x41, 0x90, 0x51, 0xaf,
    0x97, 0xcf, 0x07, 0x0c, 0x06, 0x00, 0xc7, 0x57,
    0x06, 0x00, 0x0f, 0xc1, 0xc8, 0x07, 0x70, 0x06,
    0x15, 0xcf, 0x00, 0x0c, 0x02, 0x00, 0x00, 0xda,
    0x40, 0xd1, 0x27, 0x00, 0xc2, 0x06, 0x1e, 0xcf,
    0x1d, 0xcf, 0x27, 0x0c, 0x02, 0x00, 0xcc, 0x06,
    0x19, 0xcf, 0x27, 0x02, 0x20, 0x01, 0xe7, 0x07,
    0x08, 0x00, 0x22, 0x01, 0xe7, 0x07, 0x13, 0x00,
    0xb0, 0xc0, 0x97, 0xcf, 0x41, 0x90, 0x67, 0x00,
    0x7e, 0x06, 0xe7, 0x01, 0x82, 0x06, 0x27, 0x02,
    0x80, 0x06, 0xe7, 0x07, 0x8c, 0x09, 0x84, 0x06,
    0xc8, 0x07, 0x7e, 0x06, 0xc1, 0x07, 0x00, 0x80,
    0x50, 0xaf, 0x97, 0xcf, 0x59, 0xaf, 0x97, 0xcf,
    0x00, 0x60, 0x05, 0xc0, 0xe7, 0x07, 0x00, 0x00,
    0xc4, 0x06, 0xa7, 0xcf, 0x7c, 0x06, 0x9f, 0xaf,
    0x00, 0x0a, 0xe7, 0x07, 0x01, 0x00, 0xc4, 0x06,
    0x49, 0xaf, 0xd7, 0x09, 0x00, 0xc0, 0x07, 0xaf,
    0xe7, 0x05, 0x00, 0xc0, 0x4a, 0xaf, 0xa7, 0xcf,
    0x7c, 0x06, 0xc0, 0x07, 0xfe, 0x7f, 0x44, 0xaf,
    0x40, 0x00, 0xc0, 0x37, 0x00, 0x01, 0x41, 0x90,
    0xc0, 0x37, 0x08, 0x00, 0xdf, 0xde, 0x50, 0x06,
    0xc0, 0x57, 0x10, 0x00, 0x02, 0xc2, 0xc0, 0x07,
    0x10, 0x00, 0x27, 0x00, 0x9a, 0x06, 0x41, 0x90,
    0x9f, 0xde, 0x40, 0x06, 0x44, 0xaf, 0x27, 0x00,
    0x9c, 0x06, 0xc0, 0x09, 0x9a, 0x06, 0x41, 0x90,
    0x00, 0xd2, 0x00, 0xd8, 0x9f, 0xde, 0x08, 0x00,
    0x44, 0xaf, 0x27, 0x00, 0xc8, 0x06, 0x97, 0xcf,
    0xe7, 0x87, 0x00, 0x84, 0x28, 0xc0, 0xe7, 0x67,
    0xff, 0xfb, 0x24, 0xc0, 0x97, 0xcf, 0xe7, 0x87,
    0x01, 0x00, 0xd2, 0x06, 0xe7, 0x57, 0x00, 0x00,
    0xa8, 0x06, 0x97, 0xc1, 0x9f, 0xaf, 0x00, 0x0a,
    0xe7, 0x87, 0x00, 0x06, 0x22, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0x90, 0xc0, 0xe7, 0x67, 0xfe, 0xff,
    0x3e, 0xc0, 0xe7, 0x07, 0x26, 0x00, 0x0a, 0xc0,
    0xe7, 0x87, 0x01, 0x00, 0x3e, 0xc0, 0xe7, 0x07,
    0xff, 0xff, 0xbe, 0x06, 0x9f, 0xaf, 0x10, 0x0b,
    0x97, 0xcf, 0x17, 0x00, 0xa7, 0xaf, 0x78, 0x06,
    0xc0, 0x05, 0x27, 0x00, 0x76, 0x06, 0xe7, 0x87,
    0x01, 0x00, 0xd2, 0x06, 0x9f, 0xaf, 0x00, 0x0a,
    0xe7, 0x07, 0x0c, 0x00, 0x40, 0xc0, 0x9f, 0xaf,
    0x10, 0x0b, 0x00, 0x90, 0x27, 0x00, 0xa6, 0x06,
    0x27, 0x00, 0xaa, 0x06, 0xe7, 0x09, 0xb2, 0x06,
    0xb4, 0x06, 0x27, 0x00, 0xae, 0x06, 0x27, 0x00,
    0xac, 0x06, 0x9f, 0xaf, 0xae, 0x0a, 0xc0, 0x07,
    0x00, 0x00, 0x27, 0x00, 0xb2, 0x02, 0x27, 0x00,
    0xb4, 0x02, 0x27, 0x00, 0x8e, 0x06, 0xc0, 0x07,
    0x06, 0x00, 0xc8, 0x09, 0xde, 0x00, 0xc8, 0x17,
    0x03, 0x00, 0xc9, 0x07, 0x70, 0x06, 0x29, 0x0a,
    0x00, 0xda, 0x7d, 0xc1, 0x97, 0xcf, 0xd7, 0x09,
    0x00, 0xc0, 0xc1, 0xdf, 0x00, 0x90, 0x27, 0x00,
    0x96, 0x06, 0xe7, 0x07, 0x96, 0x06, 0x98, 0x06,
    0x27, 0x00, 0xa0, 0x06, 0xe7, 0x07, 0xa0, 0x06,
    0xa2, 0x06, 0x27, 0x00, 0xa6, 0x06, 0x27, 0x00,
    0x90, 0x06, 0x27, 0x00, 0x9e, 0x06, 0xc8, 0x09,
    0x9c, 0x06, 0xc1, 0x09, 0x9a, 0x06, 0xc9, 0x07,
    0xa4, 0x06, 0x11, 0x02, 0x09, 0x02, 0xc8, 0x17,
    0x40, 0x06, 0x01, 0xda, 0x7a, 0xc1, 0x51, 0x94,
    0xc8, 0x09, 0xc8, 0x06, 0xc9, 0x07, 0xc6, 0x06,
    0xc1, 0x09, 0x9a, 0x06, 0x11, 0x02, 0x09, 0x02,
    0xc8, 0x17, 0x08, 0x00, 0x01, 0xda, 0x7a, 0xc1,
    0x51, 0x94, 0xe7, 0x05, 0x00, 0xc0, 0x97, 0xcf,
    0xe7, 0x57, 0x00, 0x00, 0x76, 0x06, 0x97, 0xc0,
    0x9f, 0xaf, 0x04, 0x00, 0xe7, 0x09, 0xbe, 0x06,
    0xba, 0x06, 0xe7, 0x57, 0xff, 0xff, 0xba, 0x06,
    0x04, 0xc1, 0xe7, 0x07, 0x10, 0x0b, 0xb8, 0x06,
    0x97, 0xcf, 0xe7, 0x17, 0x32, 0x00, 0xba, 0x06,
    0xe7, 0x67, 0xff, 0x07, 0xba, 0x06, 0xe7, 0x07,
    0x46, 0x0b, 0xb8, 0x06, 0x97, 0xcf, 0xe7, 0x57,
    0x00, 0x00, 0xc0, 0x06, 0x23, 0xc0, 0xe7, 0x07,
    0x04, 0x00, 0x90, 0xc0, 0xe7, 0x07, 0x00, 0x80,
    0x80, 0xc0, 0xe7, 0x07, 0x00, 0x00, 0x80, 0xc0,
    0xe7, 0x07, 0x00, 0x80, 0x80, 0xc0, 0xc0, 0x07,
    0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xc0, 0x07,
    0x00, 0x00, 0xe7, 0x07, 0x00, 0x00, 0x80, 0xc0,
    0xe7, 0x07, 0x00, 0x80, 0x80, 0xc0, 0xe7, 0x07,
    0x00, 0x80, 0x40, 0xc0, 0xc0, 0x07, 0x00, 0x00,
    0xe7, 0x07, 0x00, 0x00, 0x40, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0x80, 0xc0, 0xe7, 0x07, 0x04, 0x00,
    0x90, 0xc0, 0xe7, 0x07, 0x00, 0x02, 0x40, 0xc0,
    0xe7, 0x07, 0x0c, 0x02, 0x40, 0xc0, 0xe7, 0x07,
    0x00, 0x00, 0xc0, 0x06, 0xe7, 0x07, 0x00, 0x00,
    0xb8, 0x06, 0xe7, 0x07, 0x00, 0x00, 0xd2, 0x06,
    0xd7, 0x09, 0x00, 0xc0, 0xc1, 0xdf, 0x9f, 0xaf,
    0x34, 0x02, 0xe7, 0x05, 0x00, 0xc0, 0x9f, 0xaf,
    0xc4, 0x01, 0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0,
    0x17, 0x00, 0x17, 0x02, 0x97, 0x02, 0xe7, 0x57,
    0x00, 0x00, 0xa8, 0x06, 0x06, 0xc0, 0xc0, 0x09,
    0x92, 0xc0, 0xc0, 0x77, 0x09, 0x02, 0x9f, 0xc1,
    0x5c, 0x05, 0x9f, 0xcf, 0x32, 0x06, 0xd7, 0x09,
    0x0e, 0xc0, 0xe7, 0x07, 0x00, 0x00, 0x0e, 0xc0,
    0x9f, 0xaf, 0x02, 0x0c, 0xe7, 0x05, 0x0e, 0xc0,
    0x97, 0xcf, 0xd7, 0x09, 0x00, 0xc0, 0x17, 0x02,
    0xc8, 0x09, 0xb0, 0xc0, 0xe7, 0x67, 0xfe, 0x7f,
    0xb0, 0xc0, 0xc8, 0x77, 0x00, 0x20, 0x9f, 0xc1,
    0x64, 0xeb, 0xe7, 0x57, 0x00, 0x00, 0xc8, 0x02,
    0x9f, 0xc1, 0x80, 0xeb, 0xc8, 0x99, 0xca, 0x02,
    0xc8, 0x67, 0x04, 0x00, 0x9f, 0xc1, 0x96, 0xeb,
    0x9f, 0xcf, 0x4c, 0xeb, 0xe7, 0x07, 0x00, 0x00,
    0xa6, 0xc0, 0xe7, 0x09, 0xb0, 0xc0, 0xc8, 0x02,
    0xe7, 0x07, 0x03, 0x00, 0xb0, 0xc0, 0x97, 0xcf,
    0xc0, 0x09, 0xb0, 0x06, 0xc0, 0x37, 0x01, 0x00,
    0x97, 0xc9, 0xc9, 0x09, 0xb2, 0x06, 0x02, 0x00,
    0x41, 0x90, 0x48, 0x02, 0xc9, 0x17, 0x06, 0x00,
    0x9f, 0xaf, 0x08, 0x04, 0x9f, 0xa2, 0x72, 0x0c,
    0x02, 0xda, 0x77, 0xc1, 0x41, 0x60, 0x71, 0xc1,
    0x97, 0xcf, 0x17, 0x02, 0x57, 0x02, 0x43, 0x04,
    0x21, 0x04, 0xe0, 0x00, 0x43, 0x04, 0x21, 0x04,
    0xe0, 0x00, 0x43, 0x04, 0x21, 0x04, 0xe0, 0x00,
    0xc1, 0x07, 0x01, 0x00, 0xc9, 0x05, 0xc8, 0x05,
    0x97, 0xcf, 0xe7, 0x07, 0x01, 0x00, 0x8e, 0x06,
    0xc8, 0x07, 0x86, 0x06, 0xe7, 0x07, 0x00, 0x00,
    0x86, 0x06, 0xe7, 0x07, 0x10, 0x08, 0x88, 0x06,
    0xe7, 0x07, 0x04, 0x00, 0x8a, 0x06, 0xe7, 0x07,
    0xbc, 0x0c, 0x8c, 0x06, 0xc1, 0x07, 0x03, 0x80,
    0x50, 0xaf, 0x97, 0xcf, 0xe7, 0x07, 0x00, 0x00,
    0x8e, 0x06, 0x97, 0xcf,
    0x00, 0x00
};

/****************************************************************
 *     kaweth_new_code_fix
 ****************************************************************/
static __u8 kaweth_new_code_fix[] = 
{
    0xB6, 0xC3, 0xAA, 0xBB, 0xCC, 0xDD,
    0x02, 0x00, 0x08, 0x00, 0x28, 0x00, 0x2c, 0x00,
    0x34, 0x00, 0x3c, 0x00, 0x40, 0x00, 0x48, 0x00,
    0x54, 0x00, 0x58, 0x00, 0x5e, 0x00, 0x64, 0x00,
    0x68, 0x00, 0x6e, 0x00, 0x6c, 0x00, 0x72, 0x00,
    0x76, 0x00, 0x7c, 0x00, 0x80, 0x00, 0x86, 0x00,
    0x8a, 0x00, 0x90, 0x00, 0x94, 0x00, 0x98, 0x00,
    0x9e, 0x00, 0xa6, 0x00, 0xaa, 0x00, 0xb0, 0x00,
    0xb4, 0x00, 0xb8, 0x00, 0xc0, 0x00, 0xc6, 0x00,
    0xca, 0x00, 0xd0, 0x00, 0xd4, 0x00, 0xd8, 0x00,
    0xe0, 0x00, 0xde, 0x00, 0xe8, 0x00, 0xf0, 0x00,
    0xfc, 0x00, 0x04, 0x01, 0x0a, 0x01, 0x18, 0x01,
    0x22, 0x01, 0x28, 0x01, 0x3a, 0x01, 0x3e, 0x01,
    0x7e, 0x01, 0x98, 0x01, 0x9c, 0x01, 0xa2, 0x01,
    0xac, 0x01, 0xb2, 0x01, 0xba, 0x01, 0xc0, 0x01,
    0xc8, 0x01, 0xd0, 0x01, 0xd6, 0x01, 0xf4, 0x01,
    0xfc, 0x01, 0x08, 0x02, 0x16, 0x02, 0x1a, 0x02,
    0x22, 0x02, 0x2a, 0x02, 0x2e, 0x02, 0x3e, 0x02,
    0x44, 0x02, 0x4a, 0x02, 0x50, 0x02, 0x64, 0x02,
    0x62, 0x02, 0x6c, 0x02, 0x72, 0x02, 0x86, 0x02,
    0x8c, 0x02, 0x90, 0x02, 0x9e, 0x02, 0xbc, 0x02,
    0xd0, 0x02, 0xd8, 0x02, 0xdc, 0x02, 0xe0, 0x02,
    0xe8, 0x02, 0xe6, 0x02, 0xf4, 0x02, 0xfe, 0x02,
    0x04, 0x03, 0x0c, 0x03, 0x28, 0x03, 0x7c, 0x03,
    0x90, 0x03, 0x94, 0x03, 0x9c, 0x03, 0xa2, 0x03,
    0xc0, 0x03, 0xd0, 0x03, 0xd4, 0x03, 0xee, 0x03,
    0xfa, 0x03, 0xfe, 0x03, 0x2e, 0x04, 0x32, 0x04,
    0x3c, 0x04, 0x40, 0x04, 0x4e, 0x04, 0x76, 0x04,
    0x7c, 0x04, 0x84, 0x04, 0x8a, 0x04, 0x8e, 0x04,
    0xa6, 0x04, 0xb0, 0x04, 0xb8, 0x04, 0xbe, 0x04,
    0xd2, 0x04, 0xdc, 0x04, 0xee, 0x04, 0x10, 0x05,
    0x1a, 0x05, 0x24, 0x05, 0x2a, 0x05, 0x36, 0x05,
    0x34, 0x05, 0x3c, 0x05, 0x42, 0x05, 0x64, 0x05,
    0x6a, 0x05, 0x6e, 0x05, 0x86, 0x05, 0x22, 0x06,
    0x26, 0x06, 0x2c, 0x06, 0x30, 0x06, 0x42, 0x06,
    0x4a, 0x06, 0x4e, 0x06, 0x56, 0x06, 0x54, 0x06,
    0x5a, 0x06, 0x60, 0x06, 0x66, 0x06, 0xe8, 0x06,
    0xee, 0x06, 0xf4, 0x06, 0x16, 0x07, 0x26, 0x07,
    0x2c, 0x07, 0x32, 0x07, 0x36, 0x07, 0x3a, 0x07,
    0x3e, 0x07, 0x52, 0x07, 0x56, 0x07, 0x5a, 0x07,
    0x64, 0x07, 0x76, 0x07, 0x7a, 0x07, 0x80, 0x07,
    0x84, 0x07, 0x8a, 0x07, 0x9e, 0x07, 0xa2, 0x07,
    0xda, 0x07, 0xde, 0x07, 0xe2, 0x07, 0xe6, 0x07,
    0xea, 0x07, 0xee, 0x07, 0xf2, 0x07, 0xf6, 0x07,
    0x0e, 0x08, 0x16, 0x08, 0x18, 0x08, 0x1a, 0x08,
    0x1c, 0x08, 0x1e, 0x08, 0x20, 0x08, 0x22, 0x08,
    0x24, 0x08, 0x26, 0x08, 0x28, 0x08, 0x2a, 0x08,
    0x2c, 0x08, 0x2e, 0x08, 0x32, 0x08, 0x3a, 0x08,
    0x46, 0x08, 0x4e, 0x08, 0x54, 0x08, 0x5e, 0x08,
    0x78, 0x08, 0x7e, 0x08, 0x82, 0x08, 0x86, 0x08,
    0x8c, 0x08, 0x90, 0x08, 0x98, 0x08, 0x9e, 0x08,
    0xa4, 0x08, 0xaa, 0x08, 0xb0, 0x08, 0xae, 0x08,
    0xb4, 0x08, 0xbe, 0x08, 0xc4, 0x08, 0xc2, 0x08,
    0xca, 0x08, 0xc8, 0x08, 0xd4, 0x08, 0xe4, 0x08,
    0xe8, 0x08, 0xf6, 0x08, 0x14, 0x09, 0x12, 0x09,
    0x1a, 0x09, 0x20, 0x09, 0x26, 0x09, 0x24, 0x09,
    0x2a, 0x09, 0x3e, 0x09, 0x4c, 0x09, 0x56, 0x09,
    0x70, 0x09, 0x74, 0x09, 0x78, 0x09, 0x7e, 0x09,
    0x7c, 0x09, 0x82, 0x09, 0x98, 0x09, 0x9c, 0x09,
    0xa0, 0x09, 0xa6, 0x09, 0xb8, 0x09, 0xdc, 0x09,
    0xe8, 0x09, 0xec, 0x09, 0xfc, 0x09, 0x12, 0x0a,
    0x18, 0x0a, 0x1e, 0x0a, 0x42, 0x0a, 0x46, 0x0a,
    0x4e, 0x0a, 0x54, 0x0a, 0x5a, 0x0a, 0x5e, 0x0a,
    0x68, 0x0a, 0x6e, 0x0a, 0x72, 0x0a, 0x78, 0x0a,
    0x76, 0x0a, 0x7c, 0x0a, 0x80, 0x0a, 0x84, 0x0a,
    0x94, 0x0a, 0xa4, 0x0a, 0xb8, 0x0a, 0xbe, 0x0a,
    0xbc, 0x0a, 0xc2, 0x0a, 0xc8, 0x0a, 0xc6, 0x0a,
    0xcc, 0x0a, 0xd0, 0x0a, 0xd4, 0x0a, 0xd8, 0x0a,
    0xdc, 0x0a, 0xe0, 0x0a, 0xf2, 0x0a, 0xf6, 0x0a,
    0xfa, 0x0a, 0x14, 0x0b, 0x1a, 0x0b, 0x20, 0x0b,
    0x1e, 0x0b, 0x26, 0x0b, 0x2e, 0x0b, 0x2c, 0x0b,
    0x36, 0x0b, 0x3c, 0x0b, 0x42, 0x0b, 0x40, 0x0b,
    0x4a, 0x0b, 0xaa, 0x0b, 0xb0, 0x0b, 0xb6, 0x0b,
    0xc0, 0x0b, 0xc8, 0x0b, 0xda, 0x0b, 0xe8, 0x0b,
    0xec, 0x0b, 0xfa, 0x0b, 0x4a, 0x0c, 0x54, 0x0c,
    0x62, 0x0c, 0x66, 0x0c, 0x96, 0x0c, 0x9a, 0x0c,
    0xa0, 0x0c, 0xa6, 0x0c, 0xa4, 0x0c, 0xac, 0x0c,
    0xb2, 0x0c, 0xb0, 0x0c, 0xc0, 0x0c,
    0x00, 0x00
};


static const int len_kaweth_trigger_code = sizeof(kaweth_trigger_code);
static const int len_kaweth_trigger_code_fix = sizeof(kaweth_trigger_code_fix);
static const int len_kaweth_new_code = sizeof(kaweth_new_code);
static const int len_kaweth_new_code_fix = sizeof(kaweth_new_code_fix);
