#ifndef GFXCRUNCH_HPP
#define GFXCRUNCH_HPP
/*
gfxcrunch.hpp
2014/10/05
psycommando@gmail.com
Description: Main code for the Sprite utility !
*/
#include <ppmdu/basetypes.hpp>
#include <ppmdu/utils/cmdline_util.hpp>

namespace gfx_util
{
    struct pathwrapper_t;

    /*
        CSpriteUtil
            The commandline application that handles pmd2 sprites.
    */
    class CGfxUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CGfxUtil & GetInstance();

        // -- Overrides --
        //Those return their implementation specific arguments, options, and extra parameter lists.
        const std::vector<utils::cmdl::argumentparsing_t> & getArgumentsList()const;
        const std::vector<utils::cmdl::optionparsing_t>   & getOptionsList()const;
        const utils::cmdl::argumentparsing_t              * getExtraArg()const; //Returns nullptr if there is no extra arg. Extra args are args preceeded by a "+" character, usually used for handling files in batch !
       
        //For writing the title and readme!
        const std::string & getTitle()const;            //Name/Title of the program to put in the title!
        const std::string & getExeName()const;          //Name of the executable file!
        const std::string & getVersionString()const;    //Version number
        const std::string & getShortDescription()const; //Short description of what the program does for the header+title
        const std::string & getLongDescription()const;  //Long description of how the program works
        const std::string & getMiscSectionText()const;  //Text for copyrights, credits, thanks, misc..

        //Main method
        int Main(int argc, const char * argv[]);
    
    private:
        //Constructor stuff
        CGfxUtil();
        void _Construct();
        //Disable copy and move
        CGfxUtil( const CGfxUtil & );
        CGfxUtil( CGfxUtil && );
        CGfxUtil& operator=(const CGfxUtil&);
        CGfxUtil& operator=(const CGfxUtil&&);

        //Parsing methods
        bool ParseInputPath  ( const std::string              & path );
        bool ParseOutputPath ( const std::string              & path );
        bool ParseOptionQuiet( const std::vector<std::string> & optdata );

        //Execution
        int UnpackSprite();
        int BuildSprite();

        //Utility
        bool isValid_WAN_InputFile      ( const std::string & path );
        bool isValid_WANT_InputDirectory( const std::string & path );

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        //The operations that can be done by the program
        enum struct eExecMode
        {
            INVALID_Mode,
            UNPACK_SPRITE_Mode,
            BUILD_SPRITE_Mode,
            EXPORT_BGP_Mode,
            IMPORT_BGP_Mode,
            EXPORT_WTE_Mode,
            IMPORT_WTE_Mode,
        };

        //Program Settings
        bool            m_bQuiet;                       //Whether we should output to console 
        eExecMode       m_execMode;                     //This is set after reading the input path.
        std::unique_ptr<pathwrapper_t> m_pInputPath;    //This is the input path that was parsed 
        std::unique_ptr<pathwrapper_t> m_pOutputPath;   //This is the output path that was parsed

    };
};

#endif