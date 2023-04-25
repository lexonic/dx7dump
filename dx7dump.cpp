// Yamaha DX7 Sysex Dump
// Copyright 2012, Ted Felix
// License: GPLv3+

// Takes a Yamaha DX7 voice-bank sysex file and formats it as human readable text.
// The format is also conducive to using diff (or meld) to examine differences
// between patches.

// Based on info from:
// http://homepages.abdn.ac.uk/mth192/pages/dx7/sysex-format.txt

// Build:
//   g++ -o dx7dump dx7dump.cpp

// Updates by B.Lex:
// 2023-02-16: option -p implemented
// 2023-02-17: option -c implemented
// 2023-02-28: translate LCD-characters to ASCII, accept raw-format
// 2023-03-02: compiler-switch to select ASCII or UNICODE for displaying LCD content  

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

// disable to use 7-bit ASCII LCD translation
#define USE_UNICODE			// ASCII or UNICODE to be used for displaying LCD content 

// ***************************************************************************

const char *version = "1.01";

const unsigned sysexSize = 4104;
const unsigned rawDataSize = 4096;

const unsigned singleSysexSize = 163;


// "-d" to enable debugging
bool debug = false;

// "-n" don't print any listings
bool noListing = false;

// "-l" for long listing
bool longListing = false;

// "-c" for compact listing
bool compactListing = false;

// "-f" to find duplicate patches
bool findDupes = false;

// "-p" to specify patch.  -1 means all.
int patch = -1;

// "--fix" try to fix corrupted files
bool fixFiles = false;

// "-y" to say "yes" to fix files
bool fixYes = false;

// "-no-backup" don't create backups when fixing files
bool noBackup = false;

// if file has no sysex header, it might be a raw file
bool sysexFile = true;

// the active file seems corrupted and needs a fix
bool fixNeeded = false;

// file is a single voice file
bool singleVoiceFile = false;

#ifdef USE_UNICODE
char name[41];

// LCD-character translation table to UNICODE
const char lcdTable[][4] = {
    "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈",   // 0x00
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",   // 0x10
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",   // 0x20
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",   // 0x30
    "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",   // 0x40
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "¥", "]", "^", "_",   // 0x50
    "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",   // 0x60
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "→", "←",   // 0x70
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",   // 0x80
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",   // 0x90
    " ", "∘", "⌈", "⌋", "~", "⋅", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~",   // 0xA0
    "-", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~",   // 0xB0
    "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~",   // 0xC0
    "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "~", "°",   // 0xD0
    "∝", "ä", "ß", "ε", "μ", "σ", "ρ", "g", "√", "~", "j", "×", "¢", "₤", "ñ", "ö",   // 0xE0
    "p", "q", "ϴ", "∞", "Ω", "ü", "Σ", "π", "ẍ", "y", "~", "~", "~", "÷", " ", "█",   // 0xF0
};
#else
char name[11];

// LCD-character translation table for 7-bit ASCII (voice-name in sysex is also 7-bit)
const char lcdTable[] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   // 0x00
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   // 0x10
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',   // 0x20
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',   // 0x30
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',   // 0x40
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', 'Y', ']', '^', '_',   // 0x50
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',   // 0x60
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '>', '<',   // 0x70
};
#endif

// ***************************************************************************

// SYSEX Message: Bulk Data for 32 Voices

struct OperatorPacked
{
    unsigned char EG_R1;
    unsigned char EG_R2;
    unsigned char EG_R3;
    unsigned char EG_R4;
    unsigned char EG_L1;
    unsigned char EG_L2;
    unsigned char EG_L3;
    unsigned char EG_L4;
    unsigned char levelScalingBreakPoint;
    unsigned char scaleLeftDepth;
    unsigned char scaleRightDepth;

    unsigned char scaleLeftCurve : 2;  // VERIFIED
    unsigned char scaleRightCurve : 2;  // VERIFIED
    unsigned char : 4;

    unsigned char rateScale : 3;  // VERIFIED
    unsigned char detune : 4;  // VERIFIED
    unsigned : 1;

    unsigned char amplitudeModulationSensitivity : 2;  // VERIFIED
    unsigned char keyVelocitySensitivity : 3;  // VERIFIED
    unsigned : 3;

    unsigned char outputLevel;

    unsigned char oscillatorMode : 1;  // VERIFIED
    unsigned char frequencyCoarse : 5;  // VERIFIED
    unsigned : 2;

    unsigned char frequencyFine;
};

// Yamaha DX7 Voice Bank "Bulk Dump Packed Format"
// See section F of sysex-format.txt for details.
struct VoicePacked
{
    OperatorPacked op[6];

    unsigned char pitchEGR1;
    unsigned char pitchEGR2;
    unsigned char pitchEGR3;
    unsigned char pitchEGR4;
    unsigned char pitchEGL1;
    unsigned char pitchEGL2;
    unsigned char pitchEGL3;
    unsigned char pitchEGL4;

    unsigned char algorithm : 5;  // VERIFIED
    unsigned : 3;

    unsigned char feedback : 3;  // VERIFIED
    unsigned char oscKeySync : 1;  // VERIFIED
    unsigned : 4;

    unsigned char lfoSpeed;
    unsigned char lfoDelay;
    unsigned char lfoPitchModDepth;
    unsigned char lfoAMDepth;

    unsigned char lfoSync : 1;  // VERIFIED
    unsigned char lfoWave : 3;  // Need to see S&H
    unsigned char lfoPitchModSensitivity : 4;  // VERIFIED

    unsigned char transpose;
    unsigned char name[10];
};

// sysex bulk data of 32 voice bank
struct DX7Sysex
{
    unsigned char sysexBeginF0;  // 0xF0
    unsigned char yamaha43;  // 0x43
    unsigned char subStatusAndChannel;  // 0
    unsigned char format9;  // 9 -> 32 voices
    unsigned char sizeMSB;  // 7 bits!  0x20
    unsigned char sizeLSB;  // 7 bits!  0x00

    VoicePacked voices[32];

    unsigned char checksum;
    unsigned char sysexEndF7;  // 0xF7
};

// ***************************************************************************

// SYSEX Message: Bulk Data for 1 Voices

struct OperatorUnpacked
{
    unsigned char EG_R1;
    unsigned char EG_R2;
    unsigned char EG_R3;
    unsigned char EG_R4;
    unsigned char EG_L1;
    unsigned char EG_L2;
    unsigned char EG_L3;
    unsigned char EG_L4;
    unsigned char levelScalingBreakPoint;
    unsigned char scaleLeftDepth;
    unsigned char scaleRightDepth;
    unsigned char scaleLeftCurve;
    unsigned char scaleRightCurve;
    unsigned char rateScale;
    unsigned char amplitudeModulationSensitivity;
    unsigned char keyVelocitySensitivity;
    unsigned char outputLevel;
    unsigned char oscillatorMode;
    unsigned char frequencyCoarse;
    unsigned char frequencyFine;
    unsigned char detune;
};

// Yamaha DX7 Voice Bank "Single Voice Dump & Parameter #'s"
// See section D of sysex-format.txt for details.
struct VoiceUnpacked
{
    OperatorUnpacked op[6];

    unsigned char pitchEGR1;
    unsigned char pitchEGR2;
    unsigned char pitchEGR3;
    unsigned char pitchEGR4;
    unsigned char pitchEGL1;
    unsigned char pitchEGL2;
    unsigned char pitchEGL3;
    unsigned char pitchEGL4;
    unsigned char algorithm;
    unsigned char feedback;
    unsigned char oscKeySync;
    unsigned char lfoSpeed;
    unsigned char lfoDelay;
    unsigned char lfoPitchModDepth;
    unsigned char lfoAMDepth;
    unsigned char lfoSync;
    unsigned char lfoWave;
    unsigned char lfoPitchModSensitivity;
    unsigned char transpose;
    unsigned char name[10];
	unsigned char opOnOff;	// bit6 = 0 / bit 5: OP1 / ... / bit 0: OP6
};

// sysex data of single voice
struct DX7SingleSysex
{
    unsigned char sysexBeginF0;  // 0xF0
    unsigned char yamaha43;  // 0x43
    unsigned char subStatusAndChannel;  // 0
    unsigned char format0;  // 0 -> 1 voice
    unsigned char sizeMSB;  // 7 bits!  0x01
    unsigned char sizeLSB;  // 7 bits!  0x1B

    VoiceUnpacked voice;

    unsigned char checksum;
    unsigned char sysexEndF7;  // 0xF7
};

// ***************************************************************************

const char *OnOff(unsigned x)
{
    if (x > 1)
        return "*out of range*";
        
    const char *onOff[] = { "Off", "On" };
    return onOff[x];
}

const char *Curve(unsigned x)
{
    if (x > 3)
        return "*out of range*";

    const char *curves[] = { "-LIN", "-EXP", "+EXP", "+LIN" };

    return curves[x];
}

const char *LFOWave(unsigned x)
{
    if (x > 5)
        return "*out of range*";

    const char *waves[] = { "Triangle", "Saw Down", "Saw Up",
                            "Square", "Sine", "Sample & Hold" };

    return waves[x];
}

const char *Mode(unsigned x)
{
    if (x > 1)
        return "*out of range*";

    const char *modes[] = { "Frequency (Ratio)", "Fixed Frequency (Hz)" };
    return modes[x];
}

const char *ModeCompact(unsigned x)
{
    if (x > 1)
        return "*out of range*";

    const char *modes[] = { "Freq. Ratio", "Fixed Freq." };
    return modes[x];
}

const char *Note(unsigned x)
{
    x = x % 12;
    const char *notes[] = 
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return notes[x];
}

std::string Transpose(unsigned x)
{
    if (x > 48)
        return "*out of range*";

    // Anything's better than sstream!
    char buffer[10];
    snprintf(buffer, 9, "%s%u", Note(x), x / 12 + 1);

    return buffer;
}

std::string Breakpoint(unsigned x)
{
    if (x > 99)
        return "*out of range*";
        
    char buffer[10];

    // Playing some tricks (add 12 tones, subtract one octave) to avoid
    // the fact that -3/12 rounds to 0 and messes everything up.  Shifting
    // everything up an octave and then subtracting one solves the problem.
    const int octave = ((int)x - 3 + 12) / 12 - 1;
    
    // Anything's better than sstream!
    snprintf(buffer, 9, "%s%d", Note(x + 9), octave);

    return buffer;
}

// ***************************************************************************

int processOpts(int *argc, char ***argv)
{
  struct option opts[] = {
    { "long", 0, 0, 'l' },
    { "compact", 0, 0, 'c' },
    { "find-dupes", 0, 0, 'f' },
    { "patch", 1, 0, 'p' },
    { "fix", 0, 0, 'r' },
    { "yes", 0, 0, 'y' },
    { "no-backup", 0, 0, 'k' },
    { "no-lists", 0, 0, 'n' },
    { "debug", 0, 0, 'd' },
    { "version", 0, 0, 'v' },
    { "help", 0, 0, 'h' },
    { NULL, 0, 0, 0 },
  };
  
  for (;;) {
    int i = getopt_long(*argc, *argv, "lcfp:yndvh", opts, NULL);
    if (i == -1)
      break;
      
    switch (i) {
    case 'l':
      longListing = true;
      break;
    case 'c':
      compactListing = true;
      break;
    case 'f':
      findDupes = true;
      break;
    case 'p':
      patch = strtol(optarg, NULL, 0);
      patch--;
      longListing = true;
      break;
    case 'd':
      debug = true;
      break;
    case 'r': // --fix (long option only)
      fixFiles = true;
      break;
    case 'k': // --no-backup (long option only)
      noBackup = true;
      noBackup = true;
      break;
    case 'y':
      fixYes = true;
      break;
    case 'n':
      noListing = true;
      break;
    case 'v':
      printf("dx7dump %s\n", version);
            printf("Yamaha DX7 Sysex Dump\n");
            printf("Copyright 2012, Ted Felix (GPLv3+)\n");
            printf("Modifications 2023 by B.Lex\n");
      exit(0);
    case 'h':
      printf("Usage: dx7dump [OPTIONS] FILE\n");
            printf("\n");
      printf("Options:\n");
      printf("  -l, --long          long listing (show parameter values)\n");
      printf("  -c, --compact       compact listing (can also be combined with -l)\n");
      printf("  -p NUM, --patch NUM display patch number NUM\n");
      printf("  --fix               try to fix corrupt files\n");
      printf("                        creates a backup of the original file (*.ORIG)\n");
      printf("  --no-backup         don't create backups when fixing files\n");
	  printf("                        WARNING: This option might result in data-loss!\n");
	  printf("                        make sure you already have a backup of the sysex-file\n");
      printf("  -y, --yes           no questions asked. Answer everything with YES\n");
      printf("  -n, --no-lists      don't print any listings\n");
      printf("  -d, --debug         show voice names also as hex\n");
      printf("  -v, --version       version info\n");
      printf("  -h, --help          this help\n");
      exit(0);
    default:
        printf("Unexpected option.  Try -h for help.\n");
        exit(1);
      break;
    }
  }

    // Bump to the end of the options.
  *argc -= optind;
  *argv += optind;

  return 0;
}

// ***************************************************************************

// returns checksum of data block
unsigned char Checksum(const DX7Sysex *sysex)
{
    // Start of 4096 byte data block.
    const unsigned char *p = (unsigned char *)sysex->voices;
    unsigned char sum = 0;
    for (unsigned i = 0; i < rawDataSize; ++i)
    {
        sum += (p[i] & 0x7F);
    }
    // Two's complement: Flip the bits and add 1
    sum = (~sum) + 1;
    // Mask to 7 bits
    sum &= 0x7F;
    return sum;
}

// ***************************************************************************

int FixFile(DX7Sysex *sysex, const char *filename)
{
    sysex->sysexBeginF0 = 0xF0;
    sysex->yamaha43 = 0x43;
    sysex->subStatusAndChannel = 0;
    sysex->format9 = 0x09;
    sysex->sizeMSB = 0x20;
    sysex->sizeLSB = 0;
    sysex->checksum = Checksum(sysex);
    sysex->sysexEndF7 = 0xF7;

	if (!noBackup)
	{
		// create a backup of the sysex-file
	    std::string backupFile;
	    backupFile = filename;
	    backupFile += ".ORIG";
	    if (rename(filename, backupFile.c_str()))
	    {
	        printf("File could not be renamed for backup. File-fix aborted. %s\n", strerror(errno));
	        return 1;
	    }
	}

	// fix file
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        printf("Can't open the file for writing: %s. %s\n", filename, strerror(errno));
        return 1;
    }
    if (fwrite(sysex, sysexSize, 1, file) != 1)
    {
        printf("Error writing to file: %s. %s\n", filename, strerror(errno));
        return 1;
    }
    return 0;
}

// ***************************************************************************

// Returns 0 if ok.
int Verify(const DX7Sysex *sysex)
{
    // *** Verify Header and Footer ***
    
    if (sysex->sysexBeginF0 != 0xF0)
    {
        printf("Did not find sysex start F0\n");
        return 1;
    }
    if (sysex->yamaha43 != 0x43)
    {
        printf("Did not find Yamaha 0x43\n");
        return 1;
    }
	// only checking subStatus, but not Channel
    if (sysex->subStatusAndChannel & 0xF0 != 0)
    {
        printf("Did not find substatus 0. (substatus=%d)", 
			(sysex->subStatusAndChannel & 0xF0) >> 4);
        fixNeeded = true;
        // return 1;
    }
    if (sysex->format9 != 0x09)
    {
        printf("Did not find format 9 (32 voices)\n");
        fixNeeded = true;
        // return 1;
    }
    if (sysex->sizeMSB != 0x20  ||  sysex->sizeLSB != 0)
    {
        printf("WARNING: Data byte count is not 4096. (sizeMSB=0x%X, sizeLSB=0x%X)\n", 
            sysex->sizeMSB, sysex->sizeLSB);
        fixNeeded = true;
        //return 1;
    }
    if (sysex->sysexEndF7 != 0xF7)
    {
        printf("Did not find sysex end F7\n");
        return 1;
    }

    // verify checksum 
    unsigned char sum = Checksum(sysex);
    if (sum != sysex->checksum)
    {
        printf("CHECKSUM FAILED: Should have been 0x%X\n", sum);
        fixNeeded = true;
        //return 1;
    }
    
    return 0;
}

// ***************************************************************************

// Returns 0 if ok.
int VerifySingle(const DX7SingleSysex *sysex)
{
    // *** Verify Header and Footer ***
    if (sysex->sysexBeginF0 == 0xF0 &&
    	sysex->yamaha43 == 0x43 &&
		// only checking subStatus, but not Channel
    	sysex->subStatusAndChannel & 0xF0 == 0 &&
    	sysex->format0 == 0x00 &&
    	sysex->sizeMSB == 0x01 &&
		sysex->sizeLSB == 0x1B &&
		// we are not checking checksum here
    	sysex->sysexEndF7 == 0xF7)
	{
		// basic check passed
		return 0;
    }

    return 1;
}

// ***************************************************************************

char empty[1] = "";

void OpTableRow(const char* name, char* data = empty)
{
    printf("\n%-22s |", name);
    if (data[0] == 0)
    {
        // print a blank row if no data
        for (unsigned i = 1; i < 7; i++)
        {
            printf("             |");
        }
    }
    else
    {
        printf("%s", data);
    }
}

// ***************************************************************************

void OpTableSeparator()
{
    printf("\n-----------------------+");
    for (unsigned i = 1; i < 7; i++)
    {
        printf("-------------+");
    }
}

// ***************************************************************************

void VoiceSeparator()
{
    // make voice separator same length (+1) as op-table
    printf("\n=========================");
    for (unsigned i = 1; i < 7; i++)
    {
        printf("==============");
    }
}

// ***************************************************************************

void Name2Ascii(char *nameAscii, const unsigned char *nameLcd)
{
    // convert characters from LCD to ASCII
#ifdef USE_UNICODE	
    nameAscii[0] = 0; 
    for (unsigned i = 0; i < 10; i++)
    {
        strcat(nameAscii, lcdTable[int(nameLcd[i])]);
    }
#else	
    //nameAscii = ""; 
    for (unsigned i = 0; i < 10; i++)
    {
        nameAscii[i] = lcdTable[int(nameLcd[i])];
    }
    nameAscii[10] = 0;
#endif	
}

// ***************************************************************************

void Format(const DX7Sysex *sysex, const char *filename)
{
    if (!longListing)
    {
        // voice name listing only
        unsigned rows = 32;
        unsigned columns = 1;
        char voiceDelimiter = '|';
        if (compactListing)
        {
            if (!debug)
            {
                rows = 8;
                columns = 4;
            }
            else
            {
                rows = 16;
                columns = 2;
            }   
        }
        else if (!debug)
        {
            // no delimiter for plain full voice name listing
            voiceDelimiter = ' ';
        }

        for (unsigned row = 0; row < rows; ++row)
        {
            for (unsigned column = 0; column < columns; ++column)
            {
                const unsigned voiceNum = column * rows + row;
                const VoicePacked *voice = &(sysex->voices[voiceNum]);
                Name2Ascii(name, voice->name);
                printf("%2d %c%10s%c ", voiceNum + 1, voiceDelimiter, name, voiceDelimiter);
                if (debug)
                {
                    for (unsigned i = 0; i < 10; i++)
                    {
                        printf(" %2.2x", voice->name[i]);          
                    }
                }
                if (column < columns - 1)
                    printf("         ");
            }
            printf("\n");          
        }
    }
    else
    {
        // For each voice.
        for (unsigned voiceNum = 0; voiceNum < 32; ++voiceNum)
        {
            if (patch == -1 or patch == voiceNum)
            {
                const VoicePacked *voice = &(sysex->voices[voiceNum]);
                Name2Ascii(name, voice->name);
          
                // Long Listing
          
                printf("Filename: \"%s\"\n", filename);
                printf("Voice #: %d\n", voiceNum + 1);
                printf("Name: \"%s\"", name);
                if (debug)
                {
                    // debug voice name: show name in hex
                    printf(" | ");
                    for (unsigned i = 0; i < 10; i++)
                    {
                        printf(" %2.2x", voice->name[i]);          
                    }
                }
                printf("\n\n");
                printf("Algorithm: %u\n", voice->algorithm + 1);
                printf("Feedback: %u\n", voice->feedback);
          
                printf("LFO\n");
                printf("  Wave: %s\n", LFOWave(voice->lfoWave));
                printf("  Speed: %u\n", voice->lfoSpeed);
                printf("  Delay: %u\n", voice->lfoDelay);
                printf("  Pitch Mod Depth: %u\n", voice->lfoPitchModDepth);
                printf("  Amplitude Mod Depth: %u\n", voice->lfoAMDepth);
                printf("  Key Sync: %s\n", OnOff(voice->lfoSync));  
                printf("  Pitch Mod Sensitivity: %u\n", 
                       voice->lfoPitchModSensitivity);
          
                printf("Oscillator Key Sync: %s\n", OnOff(voice->oscKeySync));
          
                printf("Pitch Envelope Generator\n");
    
                if (compactListing)
                {
                    printf("  Rate 1: %-3u   Level 1: %u\n", 
                        voice->pitchEGR1, voice->pitchEGL1);
                    printf("  Rate 2: %-3u   Level 2: %u\n", 
                        voice->pitchEGR2, voice->pitchEGL2);
                    printf("  Rate 3: %-3u   Level 3: %u\n", 
                        voice->pitchEGR3, voice->pitchEGL3);
                    printf("  Rate 4: %-3u   Level 4: %u\n", 
                        voice->pitchEGR4, voice->pitchEGL4);
                }
                else
                {
                    printf("  Rate 1: %u\n", voice->pitchEGR1);
                    printf("  Rate 2: %u\n", voice->pitchEGR2);
                    printf("  Rate 3: %u\n", voice->pitchEGR3);
                    printf("  Rate 4: %u\n", voice->pitchEGR4);
                    printf("  Level 1: %u\n", voice->pitchEGL1);
                    printf("  Level 2: %u\n", voice->pitchEGL2);
                    printf("  Level 3: %u\n", voice->pitchEGL3);
                    printf("  Level 4: %u\n", voice->pitchEGL4);
                }
    
                //printf("Transpose: %s\n", Transpose(voice->transpose).c_str());
                printf("Transpose: %d\n", voice->transpose - 24);
          
                if (compactListing)
                {
                    // buffers for operator table row data
                    char ampModSens[130] = "";
                    char oscMode[130] = "";
                    char frequency[130] = "";
                    char detune[130] = "";
                    char egR1L1[130] = "";
                    char egR2L2[130] = "";
                    char egR3L3[130] = "";
                    char egR4L4[130] = "";
                    char breakpoint[130] = "";
                    char leftCurve[130] = "";
                    char rightCurve[130] = "";
                    char leftDepth[130] = "";
                    char rightDepth[130] = "";
                    char rateScale[130] = "";
                    char outputLevel[130] = "";
                    char keyVelSens[130] = "";
                    
                    // prepare table row data for each operator
                    for (unsigned i = 0; i < 6; ++i)
                    {
                        char buffer[25];
                        const unsigned j = 5 - i;
                        sprintf(ampModSens + strlen(ampModSens), 
                            " %11u |", voice->op[j].amplitudeModulationSensitivity);
                        sprintf(oscMode + strlen(oscMode), 
                            " %11s |", ModeCompact(voice->op[j].oscillatorMode));
                        sprintf(detune + strlen(detune), 
                            " %+11d |", voice->op[j].detune - 7);
                        sprintf(egR1L1 + strlen(egR1L1), 
                            " %4u : %-4u |", voice->op[j].EG_R1, voice->op[j].EG_L1);
                        sprintf(egR2L2 + strlen(egR2L2), 
                            " %4u : %-4u |", voice->op[j].EG_R2, voice->op[j].EG_L2);
                        sprintf(egR3L3 + strlen(egR3L3), 
                            " %4u : %-4u |", voice->op[j].EG_R3, voice->op[j].EG_L3);
                        sprintf(egR4L4 + strlen(egR4L4), 
                            " %4u : %-4u |", voice->op[j].EG_R4, voice->op[j].EG_L4);
                        sprintf(breakpoint + strlen(breakpoint), 
                            " %11s |", Breakpoint(voice->op[j].levelScalingBreakPoint).c_str());
                        sprintf(leftCurve + strlen(leftCurve), 
                            " %11s |", Curve(voice->op[j].scaleLeftCurve));
                        sprintf(rightCurve + strlen(rightCurve), 
                            " %11s |", Curve(voice->op[j].scaleRightCurve));
                        sprintf(leftDepth + strlen(leftDepth), 
                            " %11u |", voice->op[j].scaleLeftDepth);
                        sprintf(rightDepth + strlen(rightDepth), 
                            " %11u |", voice->op[j].scaleRightDepth);
                        sprintf(rateScale + strlen(rateScale), 
                            " %11u |", voice->op[j].rateScale);
                        sprintf(outputLevel + strlen(outputLevel), 
                            " %11u |", voice->op[j].outputLevel);
                        sprintf(keyVelSens + strlen(keyVelSens), 
                            " %11u |", voice->op[j].keyVelocitySensitivity);
    
                        // Frequency calculation
                        // If ratio mode
                        if (voice->op[j].oscillatorMode == 0)
                        {
                            double coarse = voice->op[j].frequencyCoarse;
                            if (coarse == 0)
                                coarse = .5;
              
                            const double freq = coarse + 
                                ((double)voice->op[j].frequencyFine * coarse / 100);
                            
                            sprintf(frequency + strlen(frequency), " %11g |", freq);
                        }
                        else  // fixed mode
                        {
                            const double power = (double)(voice->op[j].frequencyCoarse % 4) +
                                                 (double)voice->op[j].frequencyFine / 100;
                            const double freq = pow(10, power);
                            sprintf(frequency + strlen(frequency), "%9g Hz |", freq);
                        }
                    }
    
                    // print operator table
                    // operator table head
                    printf("\n                       |");
                    for (unsigned i = 1; i < 7; i++)
                    {
                        printf(" Operator %u  |", i);
                    }
                    OpTableSeparator();
                    OpTableRow("Amplitude Mod Sens", ampModSens);
                    OpTableRow("Oscillator Mode", oscMode);
                    OpTableRow("Frequency", frequency);
                    OpTableRow("Detune", detune);
                    OpTableSeparator();
                    OpTableRow("Envelope Generator");
                    OpTableRow("  Rate 1 : Level 1", egR1L1);
                    OpTableRow("  Rate 2 : Level 2", egR2L2);
                    OpTableRow("  Rate 3 : Level 3", egR3L3);
                    OpTableRow("  Rate 4 : Level 4", egR4L4);
                    OpTableSeparator();
                    OpTableRow("Keyboard Level Scaling");
                    OpTableRow("  Breakpoint", breakpoint);
                    OpTableRow("  Left Curve", leftCurve);
                    OpTableRow("  Right Curve", rightCurve);
                    OpTableRow("  Left Depth", leftDepth);
                    OpTableRow("  Right Depth", rightDepth);
                    OpTableSeparator();
                    OpTableRow("Keyboard Rate Scaling", rateScale);
                    OpTableRow("Output Level", outputLevel);
                    OpTableRow("Key Velocity Sens", keyVelSens);
                    OpTableSeparator();
    
                    printf("\n");
                    // don't print voice separator on last entry
                    if (patch == -1 and voiceNum < 31)
                    {
                        VoiceSeparator();
                        printf("\n\n");
                    }
                }
                else
                {
                    // For each operator
                    for (unsigned i = 0; i < 6; ++i)
                    {
                        printf("\n");
                        printf("Operator: %u\n", i + 1);
                        
                        // They're stored in backward order.
                        const unsigned j = 5 - i;
                        const OperatorPacked &op = voice->op[j];
                        
                        printf("  Amp Mod Sensitivity: %u\n", 
                               op.amplitudeModulationSensitivity);
                        printf("  Oscillator Mode: %s\n", Mode(op.oscillatorMode));
                        // If ratio mode
                        if (op.oscillatorMode == 0)
                        {
                            double coarse = op.frequencyCoarse;
                            if (coarse == 0)
                                coarse = .5;
              
                            const double freq = coarse + 
                                ((double)op.frequencyFine * coarse / 100);
                            
                            printf("  Frequency: %g\n", freq);
                        }
                        else  // fixed mode
                        {
                            const double power = (double)(op.frequencyCoarse % 4) +
                                                 (double)op.frequencyFine / 100;
                            const double f = pow(10, power);
                            printf("  Frequency: %g Hz\n", f);
                        }
                        printf("  Detune: %+d\n", op.detune - 7);
                        printf("  Envelope Generator\n");
                        printf("    Rate 1: %u\n", op.EG_R1);
                        printf("    Rate 2: %u\n", op.EG_R2);
                        printf("    Rate 3: %u\n", op.EG_R3);
                        printf("    Rate 4: %u\n", op.EG_R4);
                        printf("    Level 1: %u\n", op.EG_L1);
                        printf("    Level 2: %u\n", op.EG_L2);
                        printf("    Level 3: %u\n", op.EG_L3);
                        printf("    Level 4: %u\n", op.EG_L4);
                        printf("  Keyboard Level Scaling\n");
                        printf("    Breakpoint: %s\n", 
                               Breakpoint(op.levelScalingBreakPoint).c_str());
                        printf("    Left Curve: %s\n", Curve(op.scaleLeftCurve));
                        printf("    Right Curve: %s\n", Curve(op.scaleRightCurve));
                        printf("    Left Depth: %u\n", op.scaleLeftDepth);
                        printf("    Right Depth: %u\n", op.scaleRightDepth);
                        printf("  Keyboard Rate Scaling: %u\n", op.rateScale);
                        printf("  Output Level: %u\n", op.outputLevel);
                        printf("  Key Velocity Sensitivity: %u\n", 
                               op.keyVelocitySensitivity);
                    }
    
                    printf("-------------------------------------------------\n");
                }
            }
        }
    }
}

// ***************************************************************************

void FindDupes(const DX7Sysex *sysex)
{
    // For each patch
    for (int i = 0; i < 31; ++i)
    {
        // For each patch after that patch
        for (int j = i + 1; j < 32; ++j)
        {
            // memcmp the patches
            // Subtract 10 to remove the name from the diff.
            int rc = memcmp(&(sysex->voices[i]), &(sysex->voices[j]), 
                            sizeof(VoicePacked) - 10);
            if (rc == 0)
                printf("Found dupe: %d and %d\n", i+1, j+1);
        }
    }
}

// ***************************************************************************

int main(int argc, char *argv[])
{
    processOpts(&argc, &argv);

    if (argc == 0)
    {
        printf("Expecting a filename.\n");
        return 1;
    }

    FILE *file = fopen(argv[0], "r");
    if (file == NULL)
    {
        printf("Can't open the file: %s\n", strerror(errno));
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    unsigned char buffer[sysexSize];

    if (size == sysexSize)
    {
        size_t objCnt = fread(buffer, sysexSize, 1, file);
        fclose(file);
        if (objCnt != 1)
        {
            printf("File read error\n");
            return 1;
        }
    }
    else if (size == rawDataSize)
    {
        // check if file could be a raw file
        size_t objCnt = fread(buffer + 6, rawDataSize, 1, file);
        fclose(file);
        if (objCnt != 1)
        {
            printf("File read error\n");
            printf("File too small (%d Bytes)\n", size);
            return 1;
        }
        printf("WARNING: file seems to be a raw-dump (%d Bytes)\n", size);
        sysexFile = false;
        fixNeeded = true;
    }
    else if (size == singleSysexSize)
    {
        size_t objCnt = fread(buffer, singleSysexSize, 1, file);
        fclose(file);
        if (objCnt != 1)
        {
            printf("File read error\n");
            return 1;
        }
		singleVoiceFile = true;
    }
    else if (size > sysexSize)
    {
        printf("File too big (%d Bytes)\n", size);
        return 1;
    }
    else
    {
        printf("File too small (%d Bytes)\n", size);
        return 1;
    }
 
 	if (singleVoiceFile)
	{
		// check only if it is a single voice sysex
		// (for now, no data analyzing for Single Voice Dumps supported)
    	DX7SingleSysex *sysex = (DX7SingleSysex *)buffer;
		if (VerifySingle(sysex))
		{
            Name2Ascii(name, sysex->voice.name);
        	printf("File is a Single Voice Dump: \"%10s\"\n", name);
		}	
		else
		{
        	printf("File too small (%d Bytes)\n", size);
		}
			
		return 1;
	}

	// voice bank sysex
    DX7Sysex *sysex = (DX7Sysex *)buffer;

    // Make sure this is a valid DX7 sysex dump.
    if (sysexFile && Verify(sysex) != 0)
        return 1;

    // Format and print the bank
	if (!noListing)
    	Format(sysex, argv[0]);

    // Fix file if neccessary
    if (fixFiles && fixNeeded)
    {
        if (!fixYes)
		{
	        char choice[2];
	        printf("Fix this file? [Y/n] ");
	        fgets (choice, sizeof(choice), stdin);
	        if (choice[0] == 'N' || choice[0] == 'n')
	            return 0;
		}

        //printf("FIXING\n");
        FixFile(sysex, argv[0]);
    }

    if (findDupes)
        FindDupes(sysex);

    return 0;
}

