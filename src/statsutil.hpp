#ifndef STATS_UTIL_HPP
#define STATS_UTIL_HPP
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/basetypes.hpp>

namespace statsutil
{
    class CStatsUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CStatsUtil & GetInstance();

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
        CStatsUtil();

        //Parse Arguments
        bool ParseInputPath  ( const std::string              & path );
        bool ParseOutputPath ( const std::string              & path );

        //Parse Options
        bool ParseOptionOutTxt       ( const std::vector<std::string> & optdata );
        bool ParseOptionExpSingleFile( const std::vector<std::string> & optdata );

        //Execution
        void DetermineOperation();
        int  Execute           ();
        int  GatherArgs        ( int argc, const char * argv[] );

        //Exec methods
        int ExportPokeStatsGrowth();
        int ImportPokeStatsGrowth();

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        enum struct eOutFormat
        {
            XML,
            TXT,
        };

        enum struct eOpMode
        {
            Invalid,
            ExportPokeStatsGrowth,
            ImportPokeStatsGrowth,
        };

        //Variables
        std::string m_inputPath;      //This is the input path that was parsed 
        std::string m_outputPath;     //This is the output path that was parsed
        eOpMode     m_operationMode;  //This holds what the program should do
        eOutFormat  m_outputFormat;   //
        bool        m_bExpSingleFile; //Whether the result will be Exported to a single file, when possible
    };
};

#endif