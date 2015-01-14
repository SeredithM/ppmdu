#include "gfxcrunch.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/fmts/wan.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <utility>
#include <atomic> 
#include <chrono>
#include <thread>
#include <future>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

using namespace ::std;
using namespace ::pmd2;
using namespace ::pmd2::graphics;
using namespace ::utils::cmdl;
using namespace ::utils::io;

namespace gfx_util
{

//=================================================================================================
// Handle Export
//=================================================================================================
    /*
        A little wrapper to avoid contaminating with the POCO library the other headers!
    */
    struct pathwrapper_t
    {
        Poco::Path mypath;
    };


//=================================================================================================
// Utility
//=================================================================================================


//=================================================================================================
//  CGfxUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CGfxUtil::Exe_Name          = "ppmd_gfxcrunch.exe";
    const string CGfxUtil::Title             = "Baz the Poochyena's PMD:EoS/T/D GfxCrunch";
    const string CGfxUtil::Version           = "0.1";
    const string CGfxUtil::Short_Description = "A utility to unpack and re-pack pmd2 sprite!";
    const string CGfxUtil::Long_Description  = 
        "#TODO";                                    //#TODO
    const string CGfxUtil::Misc_Text         = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically public domain / CC0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    const vector<argumentparsing_t> CGfxUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "The path to either a folder structure containing the data of a sprite to build, or a sprite file to unpack.",
#ifdef WIN32
            "\"c:/mysprites/sprite.wan\"",
#elif __linux__
            "\"/mysprites/sprite.wan\"",
#endif
            std::bind( &CGfxUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //optional
            true,   //guaranteed to appear in order
            "output path", 
            "The path where to output the result of the operation. Can be a folder, or a file, depending on whether we're building a sprite, or unpacking one.",
#ifdef WIN32
            "\"c:/mysprites/sprite.wan\"",
#elif __linux__
            "\"/mysprites/sprite.wan\"",
#endif
            std::bind( &CGfxUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Options Info
//------------------------------------------------
    const vector<optionparsing_t> CGfxUtil::Options_List =
    {{
        //Quiet
        {
            "q",
            0,
            "Disables console progress output.",
            "-q",
            std::bind( &CGfxUtil::ParseOptionQuiet, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Methods
//------------------------------------------------

    CGfxUtil & CGfxUtil::GetInstance()
    {
        static CGfxUtil s_util;
        return s_util;
    }

    CGfxUtil::CGfxUtil()
        :CommandLineUtility()
    {
        _Construct();
    }

    void CGfxUtil::_Construct()
    {
        m_bQuiet      = false;
        m_execMode    = eExecMode::INVALID_Mode;
        m_pInputPath.reset( new pathwrapper_t );
        m_pOutputPath.reset( new pathwrapper_t );
    }

    const vector<argumentparsing_t> & CGfxUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CGfxUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CGfxUtil::getExtraArg        ()const { return &Arguments_List.front(); }
    const string                    & CGfxUtil::getTitle           ()const { return Title;                   }
    const string                    & CGfxUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CGfxUtil::getVersionString   ()const { return Version;                 }
    const string                    & CGfxUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CGfxUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CGfxUtil::getMiscSectionText ()const { return Misc_Text;               }


    void UpdateHPBar( atomic<bool> & shouldstop, atomic<uint32_t> & parsingprogress, atomic<uint32_t> & writingprogress )
    {
        while( !shouldstop )
        {
            unsigned int nbbars = (( (parsingprogress.load() + writingprogress.load()) * 16 ) /200 ); 
            cout <<"\r[" <<setw(16) <<setfill(' ') <<string( nbbars, '=' ) <<"] " 
                 <<setw(3) <<setfill(' ') <<(parsingprogress.load() + writingprogress.load()) / 200 <<"%";

            this_thread::sleep_for( chrono::milliseconds(15) );
        }
    }

    int CGfxUtil::UnpackSprite()
    {
        atomic<uint32_t> parsingprogress(0);
        atomic<uint32_t> writingprogress(0);
        //atomic<bool>     stopupdateprogress(false);

        filetypes::Parse_WAN             parser( ReadFileToByteVector( m_pInputPath->mypath.toString() ) );
        //unique_ptr<filetypes::Write_WAN> pWriter = nullptr;

        cout <<m_pInputPath->mypath.getFileName() <<setw(25) <<setfill(' ') <<"lvl" <<m_pInputPath->mypath.depth() <<"\n"
             <<"HP:\n";

        auto sprty = parser.getSpriteType();
        if( sprty == filetypes::Parse_WAN::eSpriteType::spr4bpp )
        {
            auto sprite = parser.ParseAs4bpp();
            cout<<"\n";
        }
        else if( sprty == filetypes::Parse_WAN::eSpriteType::spr8bpp )
        {
            auto sprite = parser.ParseAs8bpp();
            cout<<"\n";
        }
        //auto myfuture = std::async( std::launch::async, UpdateHPBar, std::ref(stopupdateprogress), std::ref(parsingprogress), std::ref(writingprogress) );

        //if( parser.getSpriteType() == filetypes::Parse_WAN::eSpriteType::spr4bpp )
        //    pWriter.reset( new filetypes::Write_WAN( parser.ParseAs4bpp(&parsingprogress )) );
        //else if( parser.getSpriteType() == filetypes::Parse_WAN::eSpriteType::spr8bpp )
        //    pWriter.reset( new filetypes::Write_WAN( parser.ParseAs8bpp(&parsingprogress )) );

        //Poco::File outputdirectory( m_pOutputPath->mypath );
        //if( !outputdirectory.exists() )
        //    outputdirectory.createDirectory();

        //pWriter->WriteUnpacked(outputdirectory.path(), &writingprogress );
        //assert(false);

        //stopupdateprogress = true;
        //myfuture.get();

        return 0;
    }

    int CGfxUtil::BuildSprite()
    {
        cout <<m_pInputPath->mypath.getFileName() <<setw(25) <<setfill(' ') <<"lvl" <<m_pInputPath->mypath.depth() <<"\n"
             <<"HP:\n";

        return 0;
    }


    bool CGfxUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputPath(path);
        Poco::File inputfile(inputPath);

        //check if path exists
        if( inputfile.exists() )
        {
            if( inputfile.isFile()           && isValid_WAN_InputFile( path ) )
                m_execMode = eExecMode::UNPACK_SPRITE_Mode;
            else if( inputfile.isDirectory() && isValid_WANT_InputDirectory( path ) )
                m_execMode = eExecMode::BUILD_SPRITE_Mode;
            else
                return false; //File does not exist return failure

            m_pInputPath-> mypath = inputPath;
            m_pOutputPath->mypath = inputPath;
            m_pOutputPath->mypath.makeParent();
            return true;
        }
        return false;
    }
    
    bool CGfxUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);
        bool       success = false;

        if( outpath.isDirectory() && m_execMode == eExecMode::UNPACK_SPRITE_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        else if( outpath.isFile() && m_execMode == eExecMode::BUILD_SPRITE_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        
        return success;
    }

    bool CGfxUtil::ParseOptionQuiet( const vector<string> & optdata )
    {
        //If this is called, we don't need to do any additional validation!
        return m_bQuiet = true;
    }

    bool CGfxUtil::isValid_WAN_InputFile( const string & path )
    {
        Poco::Path pathtovalidate(path);

        //A very quick and simple first stage check. The true validation will happen later, when its 
        // less time consuming to do so!
        if( pathtovalidate.getExtension().compare( filetypes::WAN_FILEX ) == 0 )
            return true;

        return false;
    }

    bool CGfxUtil::isValid_WANT_InputDirectory( const string & path )
    {
        //Poco::DirectoryIterator itdir(path);
        //Search for the required files, and subfolders
        //#TODO !
        assert(false);
        return false;
    }

    //Main method
    int CGfxUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Parse arguments and options
        try
        {
            SetArguments(argc,argv);
            cout << "\"" <<m_pInputPath->mypath.getFileName() <<"\" wants to fight!\n"
                 << "Go Poochyena!\n"
                 << "Poochyena can't wait to begin!\n";
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return returnval;
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena hit herself in confusion while biting through the parameters!\nShe's in a bit of a pinch. It looks like she might cry...\n"
                 << e.what() <<endl;
            PrintReadme();
            return returnval;
        }
        
        //Exec the operation
        try
        {
            cout << "\nPoochyena used Crunch on \"" <<m_pInputPath->mypath.getFileName() <<"\"!\n";
            switch( m_execMode )
            {
                case eExecMode::BUILD_SPRITE_Mode:
                {
                    returnval = BuildSprite();
                    cout << "\nIts super-effective!!\n"
                         <<"\"" <<m_pInputPath->mypath.getFileName() <<"\" fainted!\n"
                         <<"You got \"" <<m_pOutputPath->mypath.getFileName() <<"\" for your victory!\n";
                    break;
                }
                case eExecMode::UNPACK_SPRITE_Mode:
                {
                    cout << "\nPoochyena is so in sync with your wishes that she landed a critical hit!\n";
                    returnval = UnpackSprite();
                    cout << "\nIts super-effective!!\n"
                         << "\nThe sprite's copy got shred to pieces thanks to the critical hit!\n"
                         << "The pieces landed all neatly into \"" <<m_pOutputPath->mypath.toString() <<"\"!\n";
                    break;
                }
                default:
                {
                    assert(false); //This should never happen
                }
            };

            cout <<"" << "Poochyena used Rest!\nShe went to sleep!\n";
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena almost choked while crunching the pixels! <she gave you an apologetic look>\n"
                 << e.what() <<endl;
        }

#ifdef _DEBUG
        system("pause");
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
void Tests();
void TestEncode();

int main( int argc, const char * argv[] )
{
    using namespace gfx_util;
    CGfxUtil & application = CGfxUtil::GetInstance();
    return application.Main(argc,argv);

    //Tests();
    //TestEncode();
    return 0;
}

//Encoding/decoding tests
//#include <ppmdu/fmts/sir0.hpp>
//#include  <fstream>
//
//
//void TestEncode()
//{
//    vector<uint32_t> toencode={{ 0x130000 & 0x7FFFFF }};
//    vector<uint8_t>  encoded;
//    cout <<"Encoded :\n";
//    encoded = filetypes::EncodeSIR0PtrOffsetList( toencode );
//    for( auto & entry : encoded )
//        cout <<hex <<"0x" << static_cast<unsigned short>(entry) <<"\n";
//
//    cout <<"\nRe-decoded :\n";
//    vector<uint32_t> resultdecode = filetypes::DecodeSIR0PtrOffsetList( encoded );
//
//    for( auto & entry : resultdecode )
//        cout <<hex <<"0x" <<entry <<"\n";
//
//
//    system("pause");
//}
//
//void Tests()
//{
//    std::vector<uint8_t> todecode=
//    {{
//0x04, 0x04, 0xEB, 0x4C, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 
//0x44, 0x18, 0x82, 0x70, 0x18, 0x18, 0x83, 0x30, 0x18, 0x83, 0x10, 0x18, 0x18, 0x82, 0x70, 0x18, 
//0x83, 0x30, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 
//0x83, 0x04, 0x18, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x82, 0x30, 0x18, 0x82, 0x64, 0x18, 0x83, 
//0x30, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x82, 0x70, 0x18, 0x18, 
//0x18, 0x83, 0x38, 0x18, 0x83, 0x30, 0x18, 0x18, 0x18, 0x83, 0x38, 0x18, 0x18, 0x83, 0x04, 0x18, 
//0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x83, 0x64, 0x18, 0x83, 0x70, 0x83, 0x64, 0x18, 0x83, 0x44, 
//0x18, 0x18, 0x83, 0x64, 0x18, 0x83, 0x50, 0x83, 0x10, 0x82, 0x70, 0x18, 0x83, 0x10, 0x18, 0x83, 
//0x04, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x83, 0x30, 0x18, 0x83, 0x24, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x64, 0x18, 0x83, 0x30, 0x18, 
//0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x50, 0x18, 0x83, 0x24, 0x18, 0x18, 
//0x83, 0x24, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x81, 0x64, 0x18, 0x82, 0x64, 0x18, 0x83, 0x10, 
//0x18, 0x18, 0x83, 0x44, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 
//0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x18, 0x18, 0x82, 0x70, 0x18, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x18, 0x18, 0x82, 0x50, 0x18, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 
//0x82, 0x10, 0x18, 0x18, 0x82, 0x10, 0x18, 0x18, 0x81, 0x50, 0x18, 0x81, 0x50, 0x18, 0x83, 0x10, 
//0x18, 0x18, 0x83, 0x10, 0x82, 0x70, 0x18, 0x82, 0x30, 0x82, 0x10, 0x18, 0x18, 0x81, 0x70, 0x18, 
//0x83, 0x10, 0x83, 0x10, 0x83, 0x10, 0x83, 0x10, 0x64, 0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x9B, 0x64, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x28, 0x08, 0x20, 0x08, 0x08, 0x08, 
//0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
//0x08, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x04, 0x0C, 0x04, 0x00, 
//    }};
//
//
//    
//    cout <<"Deccoding test :\n";
//
//    ofstream outputf("converted_shinx_sir0ptr_table.txt", ios::binary);
//    vector<uint32_t> result = filetypes::DecodeSIR0PtrOffsetList( todecode );
//
//    for( auto & entry : result )
//        outputf <<hex <<"0x" <<entry <<"\n";
//
//    cout <<"Re-encoding :\n";
//    /*vector<uint32_t> toencode={{ 0x130001 & 0x7FFFFF }};*/
//    vector<uint8_t>  encoded;
//    encoded = filetypes::EncodeSIR0PtrOffsetList( result );
//    ofstream outputreencodedf("re-encoded_shinx_sir0ptr_table.bin", ios::binary);
//    for( auto & entry : encoded )
//        outputreencodedf.write( reinterpret_cast<char*>(&entry),1);
//
//    cout <<"result :\n";
//    for( auto & abyte : encoded )
//        cout <<hex <<"0x" <<static_cast<unsigned short>(abyte) <<" ";
//
//    cout <<"\nRe-decoding :\n";
//    ofstream outputredecodef("redecoded_shinx_sir0ptr_table.txt", ios::binary);
//    vector<uint32_t> resultdecode = filetypes::DecodeSIR0PtrOffsetList( encoded );
//
//    for( auto & entry : resultdecode )
//        outputredecodef <<hex <<"0x" <<entry <<"\n";
//
//    system("pause");
//}