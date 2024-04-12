/*! \file dx7dump.cpp
 *  \brief Yamaha DX7 Sysex Dump.
 *  
 *  Copyright 2012, Ted Felix
 *  Modifications 2024 by B.Lex
 *  License: GPLv3+
 * 
 *  Takes a Yamaha DX7 voice-bank sysex file and formats it as human readable text.
 *  The format is also conducive to using diff (or meld) to examine differences
 *  between patches.
 * 
 *  Based on info from:
 *  http://homepages.abdn.ac.uk/mth192/pages/dx7/sysex-format.txt
 * 
 *  Build:
 *    g++ -o dx7dump dx7dump.cpp
 * 
 *  Updates by B.Lex:
 *  2024-03-17: Options -n and -o implemented
 *  2023-02-16: Option -p implemented
 *  2023-02-17: Option -c implemented
 *  2023-02-28: Translate LCD-characters to ASCII, accept headerless-format
 *  2023-03-02: Compiler-switch to select ASCII or UNICODE for displaying LCD content
 *  2023-04-20: Option -e implemented. ASCII and UNICODE can now be selected by option -a|-u
 *  2024-04-11: Default view is now tabular view.
 *              Option -l is now -d (voicedata). Option -c is now -l (with opposite meaning).
 *              Table-borders use Unicode characters if Unicode is selected.
 *              A different table layout for the first 2 tables can be compiled (TABLE_VARIANT).
 *  2024-04-12: Option -f implemented. Table formatting optimized
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include "dx7algorithms.h"

// disable to use 7-bit ASCII LCD translation
#define USE_UNICODE_DEFAULT         // ASCII or UNICODE to be used for displaying LCD content 

// compile with table variant 1 or 2
#define TABLE_VARIANT 2

// ***************************************************************************

//! program version
const char *version = "1.03a";


//! sysex file-size of a dx7 bank-dump
const unsigned sysexSize = 4104;
//! file-size of a headerless dx7 bank-dump
const unsigned rawDataSize = 4096;

//! sysex file-size of a dx7 single-voice-dump
const unsigned singleSysexSize = 163;
//! file-size of a dx7 headerless single-voice-dump
const unsigned singleRawDataSize = 155;

//! horizontal ruler position in a table (used for setting unicode border characters)
enum HorRulerPos {
	TOP,
	MIDDLE,
	BOTTOM
};

//! set by option "-x" to show some data in hexadecimal
bool showHex = false;

//! set by option "-e" print errors only
bool errorsOnly = false;

//! set by option "-d" for voice data listing
bool voiceDataList = false;

//! set by option "-l" for long listing
bool tabularListing = true;

//! set by option "-i" to find duplicate patches
bool findDupes = false;

//! set by option "-p" to specify patch number; -1 means all.
int patch = -1;

//! set by option "--fix": try to fix corrupted files
bool fixFiles = false;

//! set by option "-n": show plain sysex filenames
bool plainFilenames = false;

//! set by option "-y" to say "yes" to fix files
bool askToFix = true;

//! set by option "-no-backup": don't create backups when fixing files
bool noBackup = false;

#ifdef USE_UNICODE_DEFAULT  
#define USE_UNICODE true
#else
#define USE_UNICODE false
#endif
//! set by option "-u" or "-a" to use unicode or ascii
bool useUnicode = USE_UNICODE;

//! filesize of opened file
int fsize;

//! sysex file data buffer
unsigned char buffer[sysexSize];

//! error & info message buffer
char msgBuffer[100] = "";

//! true if we have a recoverable error
bool softError = false;

//! if file has no sysex header, it might be a raw file
bool sysexFile = true;

//! the open file seems corrupted and could need a fix
bool fixNeeded = false;

//! the open file is a single voice file
bool singleVoiceFile = false;

//! use form-feed instead of patch separator line
bool formfeed = false;

//! printable voice-name in ASCII or UNICODE 
char name[41];      // max. length required for unicode
//char name[11];        // max. length required for ASCII only

//! vertical table line symbols for ASCII and UNICODE
const char vertLineAscii[4] = "|";
const char vertLineUnicode[4] = "│";

//! actual vertical line character
const char* vl;

//! symbols for UNICODE table borders
const char tl[] = "┌";
const char tm[] = "┬";
const char tr[] = "┐";
const char ml[] = "├";
const char mm[] = "┼";
const char mr[] = "┤";
const char bl[] = "└";
const char bm[] = "┴";
const char br[] = "┘";
//const char vv[] = "│";
//const char hh[] = "─";

//! unicode character groups for left, middle, and right borders
const char leftBorder[3][4] = {"┌", "├", "└"};
const char middleBorder[3][4] = {"┬", "┼", "┴"};
const char rightBorder[3][4] = {"┐", "┤", "┘"};

//! LCD-character translation table to UNICODE
const char lcdTableUnicode[][4] = {
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

//! LCD-character translation table for 7-bit ASCII (voice-name in sysex is also 7-bit)
const char lcdTableAscii[] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   // 0x00
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',   // 0x10
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',   // 0x20
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',   // 0x30
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',   // 0x40
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', 'Y', ']', '^', '_',   // 0x50
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',   // 0x60
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '>', '<',   // 0x70
};

//! help-text  
const char usageText[] = {
    "Usage: dx7dump [OPTIONS] FILE\n"
};

const char optionsText[] = {
    "Options:\n"
    "  -d, --voicedata     show voice data lists\n"
    "  -l, --long          long listing format (one line per name or parameter)\n"
    "  -p NUM, --patch NUM show voice data list of patch number NUM\n"
	"  -f, --formfeed      use form-feed instead of patch separator line\n"
    "  --fix               try to fix corrupt files\n"
    "                        creates a backup of the original file (*.ORIG)\n"
    "  --no-backup         don't create backups when fixing files\n"
    "                        WARNING: This option might result in data-loss!\n"
    "                        make sure you already have a backup of the sysex-file\n"
    "  -n, --plain-names   print plain filenames\n"
    "  -y, --yes           no questions asked. Answer everything with YES for '--fix'\n"
    "  -e, --errors        report only files with errors\n"
    "  -x, --hex           show voice names also as HEX and print single voice data in HEX\n"
#ifdef USE_UNICODE_DEFAULT
    "  -a, --ascii         use ASCII characters for voice-names, algorithms, and tables\n"
	"                        (default = Unicode)\n"
#else
    "  -u, --unicode       use unicode charactes for voice-names, algorithms, and tables\n"
	"                        (default = ASCII)\n"
#endif
    "  -v, --version       version info\n"
    "  -h, --help          this help"
};

//! text displayed after program version
const char versionText[] = {
    "Yamaha DX7 Sysex Dump Analyzer\n"
    "Copyright 2012, Ted Felix\n"
    "Modifications 2023, 2024: Bernhard Lex\n"
    "License GPLv3+"
};


// ***************************************************************************

// SYSEX Message: Bulk Data for 32 Voices


//! Struct of one operator in packed format.
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

/*! Yamaha DX7 Voice Bank in "Bulk Dump Packed Format".
 *
 * See section F of sysex-format.txt for details.
 */
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
    unsigned char lfoPitchModSensitivity : 3;  // VERIFIED
    unsigned : 1;

    unsigned char transpose;
    unsigned char name[10];
};

//! Sysex bulk data of 32 voice banks.
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

// SYSEX Message: Bulk Data for 1 Voice


//! Struct of one operator in unpacked format.
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

/*! Yamaha DX7 Voice Bank as "Single Voice Dump & Parameter #'s".
 *
 * See section D of sysex-format.txt for details.
 */
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
};

//! Sysex data of a single voice.
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

// Functions to convert data to text


/*! Return "out of range" string.
 *
 * \return "out of range" string
 */
const char *OutOfRange()
{
    if (tabularListing)
		return "~~~";
	else
        return "*out of range*";
}

/*! Convert parameter value to ON/OFF string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
const char *OnOff(unsigned x)
{
    if (x > 1)
        return OutOfRange();
        
    const char *onOff[] = { "Off", "On" };
    return onOff[x];
}

/*! Convert parameter value to Curve string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
const char *Curve(unsigned x)
{
    if (x > 3)
        return OutOfRange();

    const char *curves[] = { "-LIN", "-EXP", "+EXP", "+LIN" };

    return curves[x];
}

/*! Convert parameter value to LFOWave string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
const char *LFOWave(unsigned x)
{
    if (x > 5)
        return OutOfRange();

    const char *waves[] = { "Triangle", "Saw Down", "Saw Up",
                            "Square", "Sine", "Sample & Hold" };

    const char *waves_tableview[] = { "Triangle", "Saw Down", "Saw Up",
                            	  "Square", "Sine", "S/H" };

    if (tabularListing)
    	return waves_tableview[x];
	else
    	return waves[x];
}

/*! Convert parameter value to Mode string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
const char *Mode(unsigned x)
{
    if (x > 1)
        return OutOfRange();

    const char *modes[] = { "Frequency (Ratio)", "Fixed Frequency (Hz)" };

    const char *modes_tableview[] = { "Ratio", "Fixed" };

    if (tabularListing)
    	return modes_tableview[x];
	else
	    return modes[x];
}

/*! Convert parameter value to Note-name string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
const char *Note(unsigned x)
{
    x = x % 12;
    const char *notes[] = 
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return notes[x];
}

/*! Convert parameter value to Transpose string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
std::string Transpose(unsigned x)
{
    if (x > 48)
        return OutOfRange();

    // Anything's better than sstream!
    char buffer[10];
    snprintf(buffer, 9, "%s%u", Note(x), x / 12 + 1);

    return buffer;
}

/*! Convert parameter value to Breakpoint string.
 *
 * \param x parameter value
 * \return pointer to value string
 */
std::string Breakpoint(unsigned x)
{
    if (x > 99)
        return OutOfRange();
        
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

/*! Process command-line options.
 *
 *  \param argc argument count
 *  \param argv argument vector
 */
void processOpts(int *argc, char ***argv)
{
    struct option opts[] = {
        { "voicedata", 0, 0, 'd' },
        { "long", 0, 0, 'l' },
        { "find-dupes", 0, 0, 'D' },	// a hidden option
        { "patch", 1, 0, 'p' },
        { "formfeed", 0, 0, 'f' },
        { "fix", 0, 0, 'F' },
        { "yes", 0, 0, 'y' },
        { "plain-names", 0, 0, 'n' },
        { "no-backup", 0, 0, 'K' },
        { "errors", 0, 0, 'e' },
        { "hex", 0, 0, 'x' },
        { "version", 0, 0, 'v' },
        { "help", 0, 0, 'h' },
#ifdef USE_UNICODE_DEFAULT
        { "ascii", 0, 0, 'a' },
#define ENCODE_ARG "a"
#else
        { "unicode", 0, 0, 'u' },
#define ENCODE_ARG "u"
#endif
        { NULL, 0, 0, 0 },
    };
  
    for (;;)
    {
        int i = getopt_long(*argc, *argv, "dlDp:fynexvoh" ENCODE_ARG, opts, NULL);
        if (i == -1)
            break;
          
        switch (i) 
        {
        case 'd':
            voiceDataList = true;
            break;
        case 'l':
            tabularListing = false;
            break;
        case 'D':  // a hidden option
            findDupes = true;
            break;
        case 'p':
            patch = strtol(optarg, NULL, 0);
            patch--;
            voiceDataList = true;
            break;
        case 'f':
            formfeed = true;
            break;
        case 'x':
            showHex = true;
            break;
        case 'F':  // --fix (long option only)
            fixFiles = true;
            break;
        case 'K':  // --no-backup (long option only)
            noBackup = true;
            break;
		case 'n':
			plainFilenames = true;
			break;
        case 'y':
            askToFix = false;
            break;
        case 'e':
            errorsOnly = true;
            break;
#ifdef USE_UNICODE_DEFAULT
        case 'a':
            useUnicode = false;
            break;
#else     
        case 'u':
            useUnicode = true;
            break;
#endif
        case 'v':
            printf("dx7dump %s\n", version);
            //printf("%s", versionText);
            puts(versionText);
            exit(0);
        case 'h':
			//printf("%s", helpText);
			puts(usageText);
        case 'o':  // a hidden option (internally used for dx7dumpall -h)
			puts(optionsText);
            exit(0);
        default:
            puts("Try -h for help.");
            exit(1);
            break;
        }
    }
  
    // Bump to the end of the options.
    *argc -= optind;
    *argv += optind;
}

// ***************************************************************************

/*! Calculate the checksum of a sysex data block.
 *
 *  \param sysex a pointer to a DX7Sysex data block
 *  \return checksum
 */
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

// TODO: combine both Checksum functions

/*! Calculate the checksum of a single voice data block.
 *
 *  \param voice a pointer to VoiceUnpacked data
 *  \param blocksize the size of the voice data
 *  \return checksum
 */
unsigned char ChecksumSingle(const VoiceUnpacked *voice, unsigned blocksize)
{
    const unsigned char *p = (unsigned char *)voice;
    unsigned char sum = 0;

    for (unsigned i = 0; i < blocksize; ++i)
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

/*! Repair corrupt sysex files.
 *
 *  \param sysex a pointer to a DX7Sysex data block
 *  \param filename a pointer to the filename
 *  \return 0 for successful fixing the file
 *          1 for a failed attempt to fix the file
 */
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

/*! Unpack a packed voice data-block.
 *
 *  \param uVoice a pointer to the generated unpacked data
 *  \param pVoice a pointer to the packed voice data
 */
void UnpackVoice(VoiceUnpacked *uVoice, const VoicePacked *pVoice)
{
    // unpack data for each operator
    for (unsigned i = 0; i < 6; ++i)
    {
        uVoice->op[i].EG_R1 = pVoice->op[i].EG_R1;
        uVoice->op[i].EG_R2 = pVoice->op[i].EG_R2;
        uVoice->op[i].EG_R3 = pVoice->op[i].EG_R3;
        uVoice->op[i].EG_R4 = pVoice->op[i].EG_R4;
        uVoice->op[i].EG_L1 = pVoice->op[i].EG_L1;
        uVoice->op[i].EG_L2 = pVoice->op[i].EG_L2;
        uVoice->op[i].EG_L3 = pVoice->op[i].EG_L3;
        uVoice->op[i].EG_L4 = pVoice->op[i].EG_L4;
        uVoice->op[i].levelScalingBreakPoint = pVoice->op[i].levelScalingBreakPoint;
        uVoice->op[i].scaleLeftDepth = pVoice->op[i].scaleLeftDepth;
        uVoice->op[i].scaleRightDepth = pVoice->op[i].scaleRightDepth;
        uVoice->op[i].scaleLeftCurve = pVoice->op[i].scaleLeftCurve;
        uVoice->op[i].scaleRightCurve = pVoice->op[i].scaleRightCurve;
        uVoice->op[i].rateScale = pVoice->op[i].rateScale;
        uVoice->op[i].amplitudeModulationSensitivity = pVoice->op[i].amplitudeModulationSensitivity;
        uVoice->op[i].keyVelocitySensitivity = pVoice->op[i].keyVelocitySensitivity;
        uVoice->op[i].outputLevel = pVoice->op[i].outputLevel;
        uVoice->op[i].oscillatorMode = pVoice->op[i].oscillatorMode;
        uVoice->op[i].frequencyCoarse = pVoice->op[i].frequencyCoarse;
        uVoice->op[i].frequencyFine = pVoice->op[i].frequencyFine;
        uVoice->op[i].detune = pVoice->op[i].detune;
    }

    // unpack remaining part of voice
    uVoice->pitchEGR1 = pVoice->pitchEGR1;
    uVoice->pitchEGR2 = pVoice->pitchEGR2;
    uVoice->pitchEGR3 = pVoice->pitchEGR3;
    uVoice->pitchEGR4 = pVoice->pitchEGR4;
    uVoice->pitchEGL1 = pVoice->pitchEGL1;
    uVoice->pitchEGL2 = pVoice->pitchEGL2;
    uVoice->pitchEGL3 = pVoice->pitchEGL3;
    uVoice->pitchEGL4 = pVoice->pitchEGL4;
    uVoice->algorithm = pVoice->algorithm;
    uVoice->feedback = pVoice->feedback;
    uVoice->oscKeySync = pVoice->oscKeySync;
    uVoice->lfoSpeed = pVoice->lfoSpeed;
    uVoice->lfoDelay = pVoice->lfoDelay;
    uVoice->lfoPitchModDepth = pVoice->lfoPitchModDepth;
    uVoice->lfoAMDepth = pVoice->lfoAMDepth;
    uVoice->lfoSync = pVoice->lfoSync;
    uVoice->lfoWave = pVoice->lfoWave;
    uVoice->lfoPitchModSensitivity = pVoice->lfoPitchModSensitivity;
    uVoice->transpose = pVoice->transpose;
    for (unsigned i = 0; i < 10; ++i)
    {
        uVoice->name[i] = pVoice->name[i];
    }

};

// ***************************************************************************

/*! Check the integrity of a sysex dump
 *
 *  \param sysex a pointer to a DX7Sysex data block
 *  \return 0 if ok
 */
int Verify(const DX7Sysex *sysex)
{
    // *** Verify Header and Footer ***
    
    if (sysex->sysexBeginF0 != 0xF0)
    {
        sprintf(msgBuffer, "Did not find sysex start F0\n");
        return 1;
    }
    if (sysex->yamaha43 != 0x43)
    {
        sprintf(msgBuffer, "Did not find Yamaha ID 0x43\n");
        return 1;
    }
    // only checking subStatus, but not Channel
    if (sysex->subStatusAndChannel & 0xF0 != 0)
    {
        sprintf(msgBuffer, "Did not find substatus 0. (substatus=%d)\n", 
            (sysex->subStatusAndChannel & 0xF0) >> 4);
        fixNeeded = true;
        // return 1;
    }
    if (sysex->format9 != 0x09)
    {
        sprintf(msgBuffer, "Did not find format 9 (32 voices)\n");
        fixNeeded = true;
        // return 1;
    }
    if (sysex->sizeMSB != 0x20  ||  sysex->sizeLSB != 0)
    {
        sprintf(msgBuffer, "WARNING: Declared data byte count is not 4096. (sizeMSB=0x%X, sizeLSB=0x%X)\n", 
            sysex->sizeMSB, sysex->sizeLSB);
        fixNeeded = true;
        //return 1;
    }
    if (sysex->sysexEndF7 != 0xF7)
    {
        sprintf(msgBuffer, "Did not find sysex end F7\n");
        return 1;
    }

    // verify checksum 
    unsigned char sum = Checksum(sysex);
    if (sum != sysex->checksum)
    {
        sprintf(msgBuffer, "CHECKSUM FAILED: Should have been 0x%X\n", sum);
        fixNeeded = true;
        //return 1;
    }
    
    return 0;
}

// ***************************************************************************

/*! Check the integrity of a single voice sysex dump
 *
 *  \param sysex a pointer to a DX7SingleSysex data block
 *  \return 0 if ok
 */
int VerifySingle(const DX7SingleSysex *sysex)
{
    // *** Verify Header and Footer ***
    unsigned error = 0;
    if (sysex->sysexBeginF0 != 0xF0)
        error += 128;
    if (sysex->yamaha43 != 0x43)
        error += 64;
    // only checking subStatus, but not Channel
    if (sysex->subStatusAndChannel & 0xF0 != 0)
        error += 32;
    if (sysex->format0 != 0x00)
        error += 16;
    if (sysex->sizeMSB != 0x01)
        error += 8;
    if (sysex->sizeLSB != 0x1B)
        error += 4;
    // we are not checking checksum here
    if (sysex->sysexEndF7 != 0xF7)
        error += 1;
        
    //printf("ERROR: 0x%2.2X\n", error);   // DEBUG

    if (!error){
        // verify checksum 
        unsigned char sum = ChecksumSingle(&sysex->voice, sizeof(VoiceUnpacked));
        //printf("CHECKSUM: calculated=0x%2.2X  file=0x%2.2X\n", sum, sysex->checksum);   // DEBUG
        if (sum != sysex->checksum)
        {
            error += 2;
            printf("CHECKSUM FAILED: Should have been 0x%2.2X\n", sum);
            //fixNeeded = true;
        }

        // basic check passed
        return 0;
    }
        
    return 1;
}

// ***************************************************************************

/*! set global vertical line character for UNICODE or ASCII
 */ 
void setVertLineChar()
{
	if (useUnicode)
		vl = vertLineUnicode;
	else
		vl = vertLineAscii;
}

// ***************************************************************************

char empty[1] = "";

/*! Print one row of an OperatorTable (compact form).
 *
 *  \param name pointer to the parameter-name of the row
 *  \param data pointer to the stringified parameters of all 6 OP's
 */
void OpTableRow(const char* name, char* data = empty)
{
    printf("\n%s %-22s%s", vl, name, vl);
    if (data[0] == 0)
    {
        // print a blank row if no data
        for (unsigned i = 0; i < 6; i++)
        {
            printf("            %s", vl);
        }
    }
    else
    {
        printf("%s", data);
    }
}

// ***************************************************************************

/*! Print the horizontal line of an OperatorTable in compact view.
 */
void OpTableSeparator(HorRulerPos pos)
{
	if (useUnicode)
	{
		const char* middle = middleBorder[pos];
	    printf("\n%s───────────────────────%s",leftBorder[pos], middleBorder[pos]);
	    for (unsigned i = 1; i < 7; i++)
	    {
			if (i == 6)
				middle = rightBorder[pos];
	        printf("────────────%s", middle);
	    }
	}
	else
	{
	    printf("\n+-----------------------+");
	    for (unsigned i = 1; i < 7; i++)
	    {
	        printf("------------+");
	    }
	}
}

// ***************************************************************************

/*! Print a separator-line between the parameters of two different voices.
 */
void VoiceSeparator()
{
	if (formfeed)
	{
		printf("\f");
	}
	else
	{
	    // make voice separator same length (-1) as op-table
	    printf("\n========================");
	    for (unsigned i = 1; i < 7; i++)
	    {
	        printf("=============");
	    }
		puts("\n");
	}
}

// ***************************************************************************

/*! Convert the name of a voice from the stored format to printable ASCII.
 *
 *  \param nameAscii a pointer to the converted ASCII string
 *  \param nameLcd a pointer to the original name string as stored in file
 */
void Name2Ascii(char *nameAscii, const unsigned char *nameLcd)
{
    if (useUnicode)
    {
        // convert characters from LCD to Unicode
        nameAscii[0] = 0; 
        for (unsigned i = 0; i < 10; i++)
        {
            strcat(nameAscii, lcdTableUnicode[int(nameLcd[i])]);
        }
    }
    else
    {
        // convert characters from LCD to ASCII
        //nameAscii = ""; 
        for (unsigned i = 0; i < 10; i++)
        {
            nameAscii[i] = lcdTableAscii[int(nameLcd[i])];
        }
        nameAscii[10] = 0;
    }
}

// ***************************************************************************

/*! Print a filename.
 *
 *  \param filename a pointer to the filename
 */
void PrintFilename(const char *filename)
{
    int n = 0;
    if (filename[0] == '.' && filename[1] == '/')
    {
        // remove leading "./" from filename
        n = 2;
    }
    if (plainFilenames)
	{
    	printf("%s\n", filename + n);
	}
	else
	{
		printf("File: \"%s\"\n", filename + n);
	}
}

// ***************************************************************************

/*! Format and print a complete bank-dump.
 *
 *  \param sysex a pointer to a DX7Sysex data block
 *  \param filename a pointer to the filename
 */
void Format(const DX7Sysex *sysex, const char *filename)
{
    if (!voiceDataList)
    {
        // voice name listing only
        unsigned rows = 32;
        unsigned columns = 1;
        char voiceDelimiter = '|';
        if (tabularListing)
        {
            if (!showHex)
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
        else if (!showHex)
        {
            // no delimiter for plain full voice name listing
            voiceDelimiter = ' ';
        }

        if (!softError)
        {
            PrintFilename(filename);
        }

        for (unsigned row = 0; row < rows; ++row)
        {
            for (unsigned column = 0; column < columns; ++column)
            {
                const unsigned voiceNum = column * rows + row;
                const VoicePacked *voice = &(sysex->voices[voiceNum]);
                Name2Ascii(name, voice->name);
                printf("%2d %c%10s%c ", voiceNum + 1, voiceDelimiter, name, voiceDelimiter);
                if (showHex)
                {
                    for (unsigned i = 0; i < 10; i++)
                    {
                        printf(" %2.2X", voice->name[i]);          
                    }
                }
                if (column < columns - 1)
                    printf("         ");
            }
            puts("");          
        }
        puts("");          
    }
    else    // voice data listing
    {
        if (softError)
            VoiceSeparator();

		const char** algorithmDiagram;
		if (useUnicode)
		{
			algorithmDiagram = algorithmDiagramUnicode;
		}
		else
		{
        	algorithmDiagram = algorithmDiagramAscii;
		}

        // For each voice.
        for (unsigned voiceNum = 0; voiceNum < 32; ++voiceNum)
        {
            if (patch == -1 or patch == voiceNum)
            {
                const VoicePacked *voice = &(sysex->voices[voiceNum]);
          
                // Long Listing
          
                PrintFilename(filename);
                printf("Voice-#: %d\n", voiceNum + 1);
                Name2Ascii(name, voice->name);
                printf("Name: \"%s\"", name);
                if (showHex)
                {
                    // voice name: show name in hex
                    printf(" | ");
                    for (unsigned i = 0; i < 10; i++)
                    {
                        printf(" %2.2X", voice->name[i]);          
                    }

                    // print single voice raw data
                    VoiceUnpacked unpackedVoice;
                    VoiceUnpacked *uVoice = &unpackedVoice;
                    UnpackVoice(uVoice, voice);
                    printf("\n\nVoice Data:"); 
                    //char* uVoiceChar = reinterpret_cast<char*>(&unpackedVoice);
                    unsigned char* uVoiceChar = (unsigned char*)(&unpackedVoice);
                    for (unsigned i = 0; i < sizeof(unpackedVoice); i++)
                    {
                        printf(" %2.2X", uVoiceChar[i]);    
                    }
                    //printf(" [ChkSum:%2.2X]", ChecksumSingle(uVoice, sizeof(unpackedVoice)));    
                    printf(" %2.2X [last byte = checksum]", ChecksumSingle(uVoice, sizeof(unpackedVoice)));    
                }

                puts("\n");
                printf("Algorithm: %u\n", voice->algorithm + 1);

                if (tabularListing)
                {
                    // print algorithm diagram as ASCII-art
                	printf("\n%s\n", algorithmDiagram[voice->algorithm]);

                	//puts("");

#if TABLE_VARIANT == 2
					// TABLE_VARIANT 2

					// +------------+-------+-------+------------+--------+
					// |            | Algo- | Feed- | Oscillator | Trans- |
					// | Voice Name | rithm | back  | Key Sync   | pose   |
					// +------------+-------+-------+------------+--------+
					if (useUnicode)
						printf("┌────────────┬───────┬───────┬────────────┬────────┐\n");
					else
						printf("+------------+-------+-------+------------+--------+\n");
					printf("%1$s            %1$s Algo- %1$s Feed- %1$s Oscillator "
						   "%1$s Trans- %1$s\n", vl);
					
					printf("%1$s Voice Name %1$s rithm %1$s back  %1$s Key Sync   "
					       "%1$s pose   %1$s\n", vl);
					if (useUnicode)
						printf("├────────────┼───────┼───────┼────────────┼────────┤\n");
					else
						printf("+------------+-------+-------+------------+--------+\n");
					printf("%s %-10s %s %5u %s %5u %s %10s %s %6d %s\n",
						vl, name, vl,
						voice->algorithm + 1, vl,
						voice->feedback, vl,
						OnOff(voice->oscKeySync), vl,
						voice->transpose - 24, vl);
					if (useUnicode)
						printf("└────────────┴───────┴───────┴────────────┴────────┘\n");
					else
						printf("+------------+-------+-------+------------+--------+\n");
                	puts("");
					if (!useUnicode)
                		puts("");

					// +----------------------------------------------------------------+-------------------------------+
					// |                              LFO                               |   Pitch Envelope Generator    |
					// +------+-------+-------+-----------+-----------+------+----------+-------+-------+-------+-------+
					// |      |       |       | Pitch     | Amplitude | Key  | Pitch    |       |       |       |       |
					// | Wave | Speed | Delay | Mod Depth | Mod Depth | Sync | Mod Sens | R1:L1 | R2:L2 | R3:L3 | R4:L4 |
					// +------+-------+-------+-----------+-----------+------+----------+-------+-------+-------+-------+
					// 
					if (useUnicode)
						printf("┌─────────────────────────────────────────────────────────────────────"
							   "┬───────────────────────────────┐\n");
					else
						printf("+---------------------------------------------------------------------"
							   "+-------------------------------+\n");
					printf("%1$s                                  LFO                                "
						   "%1$s   Pitch Envelope Generator    %1$s\n", vl);
					if (useUnicode)
						printf("├──────────┬───────┬───────┬───────────┬───────────┬───────┬──────────"
							   "┼───────┬───────┬───────┬───────┤\n");
					else
						printf("+----------+-------+-------+-----------+-----------+-------+----------"
							   "+-------+-------+-------+-------+\n");
					printf("%1$s          %1$s       %1$s       %1$s Pitch     "
						   "%1$s Amplitude %1$s Key   %1$s Pitch    "
						   "%1$s       %1$s       %1$s       %1$s       %1$s\n", vl);
					printf("%1$s Wave     %1$s Speed %1$s Delay %1$s Mod Depth "
					       "%1$s Mod Depth %1$s Sync  %1$s Mod Sens "
						   "%1$s R1:L1 %1$s R2:L2 %1$s R3:L3 %1$s R4:L4 %1$s\n", vl);
					if (useUnicode)
						printf("├──────────┼───────┼───────┼───────────┼───────────┼───────┼──────────"
							   "┼───────┼───────┼───────┼───────┤\n");
					else
						printf("+----------+-------+-------+-----------+-----------+-------+----------"
							   "+-------+-------+-------+-------+\n");
					printf("%s %8s %s %5u %s %5u %s %9u %s %9u %s %5s %s %8u "
						   "%s %2u:%-2u %s %2u:%-2u %s %2u:%-2u %s %2u:%-2u %s\n",
	                	vl, LFOWave(voice->lfoWave), vl,
	                	voice->lfoSpeed, vl,
	                	voice->lfoDelay, vl,
	                	voice->lfoPitchModDepth, vl,
	                	voice->lfoAMDepth, vl,
	                	OnOff(voice->lfoSync), vl,
	                    voice->lfoPitchModSensitivity, vl,
						voice->pitchEGR1, voice->pitchEGL1, vl,
						voice->pitchEGR2, voice->pitchEGL2, vl,
						voice->pitchEGR3, voice->pitchEGL3, vl,
						voice->pitchEGR4, voice->pitchEGL4, vl);
					if (useUnicode)
						printf("└──────────┴───────┴───────┴───────────┴───────────┴───────┴──────────"
							   "┴───────┴───────┴───────┴───────┘\n");
					else
						printf("+----------+-------+-------+-----------+-----------+-------+----------"
							   "+-------+-------+-------+-------+\n");
					if (!useUnicode)
                		puts("");

#else   
					// TABLE_VARIANT 1

					// +------------+-------+-------+------------+-------------------------------+--------+
					// |            | Algo- | Feed- | Oscillator |   Pitch Envelope Generator    | Trans- |
					// | Voice Name | rithm | back  | Key Sync   | R1:L1 | R2:L2 | R3:L3 | R4:L4 | pose   |
					// +------------+-------+-------+------------+-------+-------+-------+-------+--------+
					if (useUnicode)
						printf("┌────────────┬───────┬───────┬────────────┬───────────────────────────────┬────────┐\n");
					else
						printf("+------------+-------+-------+------------+-------------------------------+--------+\n");
					if (useUnicode)
					{
						printf("│            │       │       │            │   Pitch Envelope Generator    │        │\n");
						printf("│            │ Algo- │ Feed- │ Oscillator ├───────┬───────┬───────┬───────┤ Trans- │\n");
					}
					else
					{
						printf("%1$s            %1$s Algo- %1$s Feed- %1$s Oscillator "
							   "%1$s   Pitch Envelope Generator    %1$s Trans- %1$s\n", vl);
					}
					printf("%1$s Voice Name %1$s rithm %1$s back  %1$s Key Sync   "
					       "%1$s R1:L1 %1$s R2:L2 %1$s R3:L3 %1$s R4:L4 %1$s pose   %1$s\n", vl);
					if (useUnicode)
						printf("├────────────┼───────┼───────┼────────────┼───────┼───────┼───────┼───────┼────────┤\n");
					else
						printf("+------------+-------+-------+------------+-------+-------+-------+-------+--------+\n");
					printf("%s %-10s %s %5u %s %5u %s %10s %s %2u:%-2u %s %2u:%-2u %s %2u:%-2u %s %2u:%-2u %s %6d %s\n",
						vl, name, vl,
						voice->algorithm + 1, vl,
						voice->feedback, vl,
						OnOff(voice->oscKeySync), vl,
						voice->pitchEGR1, voice->pitchEGL1, vl,
						voice->pitchEGR2, voice->pitchEGL2, vl,
						voice->pitchEGR3, voice->pitchEGL3, vl,
						voice->pitchEGR4, voice->pitchEGL4, vl,
						voice->transpose - 24, vl);
					if (useUnicode)
						printf("└────────────┴───────┴───────┴────────────┴───────┴───────┴───────┴───────┴────────┘\n");
					else
						printf("+------------+-------+-------+------------+-------+-------+-------+-------+--------+\n");
                	puts("");
					if (!useUnicode)
                		puts("");

					// +---------------------------------------------------------------------+
					// |                                  LFO                                |
					// |          |       |       | Pitch     | Amplitude | Key   | Pitch    |
					// | Wave     | Speed | Delay | Mod Depth | Mod Depth | Sync  | Mod Sens |
					// +----------+-------+-------+-----------+-----------+-------+----------+
					// | Triangle |    30 |     0 |         8 |         0 |   Off |        2 |
					// +----------+-------+-------+-----------+-----------+-------+----------+
					if (useUnicode)
						printf("┌─────────────────────────────────────────────────────────────────────┐\n");
					else
						printf("+---------------------------------------------------------------------+\n");
					printf("%1$s                                  LFO                                %1$s\n", vl);
					if (useUnicode)
						printf("├──────────┬───────┬───────┬───────────┬───────────┬───────┬──────────┤\n");
					printf("%1$s          %1$s       %1$s       %1$s Pitch     "
						   "%1$s Amplitude %1$s Key   %1$s Pitch    %1$s\n", vl);
					printf("%1$s Wave     %1$s Speed %1$s Delay %1$s Mod Depth "
					       "%1$s Mod Depth %1$s Sync  %1$s Mod Sens %1$s\n", vl);
					if (useUnicode)
						printf("├──────────┼───────┼───────┼───────────┼───────────┼───────┼──────────┤\n");
					else
						printf("+----------+-------+-------+-----------+-----------+-------+----------+\n");
					printf("%s %8s %s %5u %s %5u %s %9u %s %9u %s %5s %s %8u %s\n",
	                	vl, LFOWave(voice->lfoWave), vl,
	                	voice->lfoSpeed, vl,
	                	voice->lfoDelay, vl,
	                	voice->lfoPitchModDepth, vl,
	                	voice->lfoAMDepth, vl,
	                	OnOff(voice->lfoSync), vl,
	                    voice->lfoPitchModSensitivity, vl);
					if (useUnicode)
						printf("└──────────┴───────┴───────┴───────────┴───────────┴───────┴──────────┘\n");
					else
						printf("+----------+-------+-------+-----------+-----------+-------+----------+\n");
					if (!useUnicode)
                		puts("");
#endif // TABLE_VARIANT
                }
				else
				{
	                printf("Feedback: %u\n", voice->feedback);
	          
	                printf("LFO\n");
	                printf("  Wave: %s\n", LFOWave(voice->lfoWave));
	                printf("  Speed: %u\n", voice->lfoSpeed);
	                printf("  Delay: %u\n", voice->lfoDelay);
	                printf("  Pitch Mod. Depth: %u\n", voice->lfoPitchModDepth);
	                printf("  Amplitude Mod. Depth: %u\n", voice->lfoAMDepth);
	                printf("  Key Sync: %s\n", OnOff(voice->lfoSync));  
	                printf("  Pitch Mod. Sensitivity: %u\n", 
	                       voice->lfoPitchModSensitivity);
	          
	                printf("Oscillator Key Sync: %s\n", OnOff(voice->oscKeySync));
	          
	                printf("Pitch Envelope Generator\n");
                    printf("  Rate 1: %u\n", voice->pitchEGR1);
                    printf("  Rate 2: %u\n", voice->pitchEGR2);
                    printf("  Rate 3: %u\n", voice->pitchEGR3);
                    printf("  Rate 4: %u\n", voice->pitchEGR4);
                    printf("  Level 1: %u\n", voice->pitchEGL1);
                    printf("  Level 2: %u\n", voice->pitchEGL2);
                    printf("  Level 3: %u\n", voice->pitchEGL3);
                    printf("  Level 4: %u\n", voice->pitchEGL4);
   
	                //printf("Transpose: %s\n", Transpose(voice->transpose).c_str());
    	            printf("Transpose: %d\n", voice->transpose - 24);
                }
 
                if (tabularListing)
                {
                    // buffers for operator table row data
                    char tableHeader[120] = "";
                    char ampModSens[120] = "";
                    char oscMode[120] = "";
                    char frequency[120] = "";
                    char detune[120] = "";
                    char egR1L1[120] = "";
                    char egR2L2[120] = "";
                    char egR3L3[120] = "";
                    char egR4L4[120] = "";
                    char breakpoint[120] = "";
                    char leftCurve[120] = "";
                    char rightCurve[120] = "";
                    char leftDepth[120] = "";
                    char rightDepth[120] = "";
                    char rateScale[120] = "";
                    char outputLevel[120] = "";
                    char keyVelSens[120] = "";
                    
                    // prepare table row data for each operator
                    for (unsigned i = 0; i < 6; ++i)
                    {
                        char buffer[25];
                        const unsigned j = 5 - i;
                        sprintf(tableHeader + strlen(tableHeader), 
                            " Operator %u %s", i + 1, vl);
                        sprintf(ampModSens + strlen(ampModSens), 
                            " %10u %s", voice->op[j].amplitudeModulationSensitivity, vl);
                        sprintf(oscMode + strlen(oscMode), 
                            " %10s %s", Mode(voice->op[j].oscillatorMode), vl);
                        sprintf(detune + strlen(detune), 
                            " %+10d %s", voice->op[j].detune - 7, vl);
                        sprintf(egR1L1 + strlen(egR1L1), 
                            " %4u : %-3u %s", voice->op[j].EG_R1, voice->op[j].EG_L1, vl);
                        sprintf(egR2L2 + strlen(egR2L2), 
                            " %4u : %-3u %s", voice->op[j].EG_R2, voice->op[j].EG_L2, vl);
                        sprintf(egR3L3 + strlen(egR3L3), 
                            " %4u : %-3u %s", voice->op[j].EG_R3, voice->op[j].EG_L3, vl);
                        sprintf(egR4L4 + strlen(egR4L4), 
                            " %4u : %-3u %s", voice->op[j].EG_R4, voice->op[j].EG_L4, vl);
                        sprintf(breakpoint + strlen(breakpoint), 
                            " %10s %s", Breakpoint(voice->op[j].levelScalingBreakPoint).c_str(), vl);
                        sprintf(leftCurve + strlen(leftCurve), 
                            " %10s %s", Curve(voice->op[j].scaleLeftCurve), vl);
                        sprintf(rightCurve + strlen(rightCurve), 
                            " %10s %s", Curve(voice->op[j].scaleRightCurve), vl);
                        sprintf(leftDepth + strlen(leftDepth), 
                            " %10u %s", voice->op[j].scaleLeftDepth, vl);
                        sprintf(rightDepth + strlen(rightDepth), 
                            " %10u %s", voice->op[j].scaleRightDepth, vl);
                        sprintf(rateScale + strlen(rateScale), 
                            " %10u %s", voice->op[j].rateScale, vl);
                        sprintf(outputLevel + strlen(outputLevel), 
                            " %10u %s", voice->op[j].outputLevel, vl);
                        sprintf(keyVelSens + strlen(keyVelSens), 
                            " %10u %s", voice->op[j].keyVelocitySensitivity, vl);
    
                        // Frequency calculation
                        // If ratio mode
                        if (voice->op[j].oscillatorMode == 0)
                        {
                            double coarse = voice->op[j].frequencyCoarse;
                            if (coarse == 0)
                                coarse = .5;
              
                            const double freq = coarse + 
                                ((double)voice->op[j].frequencyFine * coarse / 100);
                            
                            sprintf(frequency + strlen(frequency), " %10g %s", freq, vl);
                        }
                        else  // fixed mode
                        {
                            const double power = (double)(voice->op[j].frequencyCoarse % 4) +
                                                 (double)voice->op[j].frequencyFine / 100;
                            const double freq = pow(10, power);
                            sprintf(frequency + strlen(frequency), "%8g Hz %s", freq, vl);
                        }
                    }
    
                    // print operator table
                    // operator table head
                    OpTableSeparator(TOP);
                    OpTableRow("", tableHeader);
                    OpTableSeparator(MIDDLE);
                    OpTableRow("Amplitude Mod. Sens.", ampModSens);
                    OpTableRow("Oscillator Freq. Mode", oscMode);
                    OpTableRow("Frequency", frequency);
                    OpTableRow("Detune", detune);
                    OpTableSeparator(MIDDLE);
                    OpTableRow("Envelope Generator");
                    OpTableRow("  Rate 1 : Level 1", egR1L1);
                    OpTableRow("  Rate 2 : Level 2", egR2L2);
                    OpTableRow("  Rate 3 : Level 3", egR3L3);
                    OpTableRow("  Rate 4 : Level 4", egR4L4);
                    OpTableSeparator(MIDDLE);
                    OpTableRow("Keybd. Level Scaling");
                    OpTableRow("  Breakpoint", breakpoint);
                    OpTableRow("  Left Curve", leftCurve);
                    OpTableRow("  Right Curve", rightCurve);
                    OpTableRow("  Left Depth", leftDepth);
                    OpTableRow("  Right Depth", rightDepth);
                    OpTableSeparator(MIDDLE);
                    OpTableRow("Keyboard Rate Scaling", rateScale);
                    OpTableRow("Output Level", outputLevel);
                    OpTableRow("Key Velocity Sens.", keyVelSens);
                    OpTableSeparator(BOTTOM);
    
                    puts("");

                    // don't print voice separator on last entry
                    //if (patch == -1 and voiceNum < 31)

                    // don't print voice separator for a single patch 
                    if (patch == -1)
                        VoiceSeparator();
                }
                else
                {
                    // For each operator
                    for (unsigned i = 0; i < 6; ++i)
                    {
                        puts("");
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
    
                    // don't print any voice separator for a single patch
                    if (patch == -1)
                    {
                        // print big separator at EOF
                        if (voiceNum == 31)
                            VoiceSeparator();
                        else
                            puts("-------------------------------------------------\n");
                    }
                }
            }
        }
    }
}

// ***************************************************************************

/*! Find and print duplicates within a voice bank dump.
 *
 *  \param sysex a pointer to a DX7Sysex data block
 */
void FindDupes(const DX7Sysex *sysex)
{
    bool dupeFound = false;

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
            {
                printf("Found duplicate: %d = %d\n", i+1, j+1);
                dupeFound = true;
            }
        }
    }

    if (dupeFound)
    {
        puts("");
    }
}

// ***************************************************************************

/*! Process a complete voice dump sysex file.
 *
 *  \param filename a pointer to the filename
 *  \return 0 if ok
 */
int processFile (char* filename)
{
	FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("ERROR: Can't open the file: %s\n", strerror(errno));
        return 1;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    rewind(file);

    if (fsize == sysexSize)
    {
        size_t objCnt = fread(buffer, sysexSize, 1, file);
        fclose(file);
        if (objCnt != 1)
        {
            PrintFilename(filename);
            puts("File read error\n");
            return 1;
        }
    }
    else if (fsize == rawDataSize)
    {
        // check if file could be a raw file
        size_t objCnt = fread(buffer + 6, rawDataSize, 1, file);
        fclose(file);
        PrintFilename(filename);
        if (objCnt != 1)
        {
            puts("File read error");
            printf("File too small (%d Bytes)\n\n", fsize);
            return 1;
        }
        printf("WARNING: file seems to be a headerless dump (%d Bytes)\n", fsize);
        softError = true;
        sysexFile = false;
        fixNeeded = true;
    }
    else if (fsize == singleSysexSize)
    {
        size_t objCnt = fread(buffer, singleSysexSize, 1, file);
        fclose(file);
        if (objCnt != 1)
        {
            PrintFilename(filename);
            puts("File read error\n");
            return 1;
        }
        singleVoiceFile = true;
    }
    else if (fsize > sysexSize)
    {
        PrintFilename(filename);
        printf("File too big (%d Bytes)\n\n", fsize);
        return 1;
    }
    else
    {
        PrintFilename(filename);
        printf("File too small (%d Bytes)\n\n", fsize);
        return 1;
    }
 
    if (singleVoiceFile)
    {
        // check only if it is a single voice sysex
        // (for now, no data analyzing for Single Voice Dumps supported)
        DX7SingleSysex *sysex = (DX7SingleSysex *)buffer;
        PrintFilename(filename);
        if (VerifySingle(sysex) == 0)
        {
            Name2Ascii(name, sysex->voice.name);
            printf("File is a Single Voice Dump: \"%10s\"\n\n", name);
        }   
        else
        {
            printf("File too small (%d Bytes)\n\n", fsize);
        }
            
        return 1;
    }

    // voice bank sysex
    DX7Sysex *sysex = (DX7Sysex *)buffer;

    // Make sure this is a valid DX7 sysex dump.
    // there is no validation for headerless dumps!
    if (sysexFile && Verify(sysex) != 0)
    {
        // unrecoverable file error
        PrintFilename(filename);
        puts(msgBuffer);
        return 1;
    }

    if (msgBuffer[0] != 0)
    {
        // we have a recoverable file error
        softError = true;
        PrintFilename(filename);
        printf("%s", msgBuffer);
    }

    // Format and print the bank
    if (!errorsOnly)
    {
        Format(sysex, filename);
    }
    else if (softError)
    {
        puts("");
    }

    // Fix file if neccessary
    if (fixFiles && fixNeeded)
    {
        if (askToFix)
        {
            char choice[2];
            printf("Fix this file? [Y/n] ");
            fgets (choice, sizeof(choice), stdin);
            if (choice[0] == 'N' || choice[0] == 'n')
                return 0;
        }

        //printf("FIXING\n");
        FixFile(sysex, filename);
    }

    if (findDupes)
        FindDupes(sysex);

	return 0;
};




// ***************************************************************************

/*! The main function of dx7dump.
 *
 *  \param argc argument count
 *  \param argv argument vector
 */
int main(int argc, char *argv[])
{
    processOpts(&argc, &argv);

    if (argc == 0)
    {
        puts("Expecting a filename.");
        return 1;
    }

	setVertLineChar();

	if (processFile(argv[0]))
	{
		// we had errors processing the file
		return 1;
	}

    return 0;
}

