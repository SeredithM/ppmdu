#include "game_stats.hpp"
#include <utils/poco_wrapper.hpp>
#include <ppmdu/fmts/waza_p.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <ppmdu/fmts/m_level.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <ppmdu/fmts/item_p.hpp>
#include <pugixml.hpp>
#include <utils/parse_utils.hpp>
#include <utils/library_wide.hpp>
#include <Poco/Path.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
using namespace std;
using namespace pmd2::filetypes;
using namespace filetypes;

namespace pmd2{ namespace stats
{
//==========================================================================================
//
//==========================================================================================
    static const string BalanceDirectory  = "BALANCE";
    static const string GameTextDirectory = "MESSAGE";

    //std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames =
    //{
    //    "",
    //    "EoS",
    //    "EoTD",
    //};


    const string GameStats::DefPkmnDir    = "pokemon_data";
    const string GameStats::DefMvDir      = "move_data";
    const string GameStats::DefStrFName   = "game_text.txt";
    const string GameStats::DefItemsDir   = "item_data";
    const string GameStats::DefDungeonDir = "dungeon_data";

//==========================================================================================
//  Utilities
//==========================================================================================
    /*
        Test whether the directory contains XML data that
        can be imported.
    */
    bool isImportAllDir( const std::string & directory )
    {
        //Check for one of the required directories
        const string pkmndir = Poco::Path(directory).append(GameStats::DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).append(GameStats::DefItemsDir).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).append(GameStats::DefMvDir   ).makeDirectory().toString();
        return ( utils::isFolder(pkmndir) || utils::isFolder(mvdir) || utils::isFolder(itemdir) );
    }




//==========================================================================================
//  GameStats
//==========================================================================================

    GameStats::GameStats( const std::string & pmd2rootdir, const std::string & gamelangfile )
        :m_dataFolder(pmd2rootdir), m_gameVersion(eGameVersion::Invalid), m_gamelangfile(gamelangfile)
    {}

//--------------------------------------------------------------
//  Game Analysis
//--------------------------------------------------------------
    void GameStats::IdentifyGameVersion()
    {
        //To identify whether its Explorers of Sky or Explorers of Time/Darkness
        // check if we have a waza_p2.bin file under the /BALANCE/ directory.
        stringstream sstr;
        sstr << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;

        if( utils::isFolder( sstr.str() ) )
        {
            sstr << "/" << filetypes::WAZA2_Fname;
            if( utils::isFile( sstr.str() ) )
                m_gameVersion = eGameVersion::EoS;
            else
                m_gameVersion = eGameVersion::EoTEoD;
        }
        else
            throw runtime_error( "Specified data directory is invalid! : " + m_dataFolder );

        if( utils::LibWide().isLogOn() )
        {
            clog << "Detected game version : " 
                 << ( (m_gameVersion == eGameVersion::EoS )? "EoS" : ( m_gameVersion == eGameVersion::EoTEoD )? "EoT/D" : "Invalid" )
                 << "\n";
        }
    }

    void GameStats::IdentifyGameLocaleStr()
    {
        /*
            Load game language file
        */
        if( !utils::isFile(m_gamelangfile) )
            throw runtime_error("Path specified is an invalid gamelang file! : " + m_gamelangfile);
        m_possibleLang = GameLanguageLoader( m_gamelangfile, m_gameVersion );

        /*
            To identify the game language, find the text_*.str file, and compare the name to the lookup table.
        */
        static const string TextStr_FnameBeg = "text_";
        stringstream        sstr;
        sstr << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory;
        const string MessageDirPath =  sstr.str();

        if( utils::isFolder( MessageDirPath ) )
        {
            const vector<string> filelist = utils::ListDirContent_FilesAndDirs( MessageDirPath, true );
            for( const auto & fname : filelist )
            {
                //if the begining of the text_*.str filename matches, do a comparison, and return the result
                if( fname.size() > TextStr_FnameBeg.size() &&
                   fname.substr( 0, (TextStr_FnameBeg.size()) ) == TextStr_FnameBeg )
                {
                    m_gameTextFName  = fname;
                    m_gameLangLocale = m_possibleLang.FindLocaleString( fname );

                    if( utils::LibWide().isLogOn() )
                        clog << "Got locale string : \"" <<m_gameLangLocale <<"\" for text file \"" <<m_gameTextFName <<"\" !\n";
                    return;
                }
            }
        }
 
        //We end up here if we have no data on that file name, or if the file was not found at all!!

        m_gameLangLocale = ""; //Use default locale if nothing match

        if( utils::LibWide().isLogOn() )
            clog << "WARNING: No data found for game text file : \"" <<m_gameTextFName <<"\" ! Falling back to default locale!\n";
    }

    void GameStats::BuildListOfStringOffsets()
    {
        //using namespace GameLangXMLStr;
        m_strOffsets.resize(StrBlocksNames.size());
        bool         bencounteredError = false;
        stringstream sstr;

        for( unsigned int i = 0; i < StrBlocksNames.size(); ++i )
        {
            auto result = m_possibleLang.FindStrBlockOffset( StrBlocksNames[i], m_gameTextFName );
            if( result.first )
            {
                m_strOffsets[i].beg = result.second.first;
                m_strOffsets[i].end = result.second.second;
            }
            else
            {
                sstr <<"ERROR: The beginning or the end of the " <<StrBlocksNames[i] 
                     <<" string block(s) was not found in the game string location configuration file!\n\n";
                bencounteredError = true;
            }
        }

        //Print all the missing entries
        if(bencounteredError)
        {
            string strerr = sstr.str();
            clog << strerr <<"\n";
            //throw runtime_error( strerr );
        }
    }

//--------------------------------------------------------------
//  Loading
//--------------------------------------------------------------

    void GameStats::Load()
    {
        //First identify what we're dealing with
        AnalyzeGameDir();
        _LoadGameStrings();
        _LoadPokemonAndMvData();
        _LoadItemData();
    }

    void GameStats::Load( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        Load();
    }

    void GameStats::LoadStrings()
    {
        AnalyzeGameDir();
        _LoadGameStrings();
    }

    void GameStats::LoadStrings( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        LoadStrings();
    }

    void GameStats::_LoadGameStrings()
    {
        stringstream sstr;
        sstr << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;
        m_gameStrings = filetypes::ParseTextStrFile( sstr.str(), std::locale( m_gameLangLocale ) );
    }

    void GameStats::_EnsureStringsLoaded()
    {
        if( m_gameStrings.empty() )
        {
            cout<<"  <!>- Need to load target game strings file! Loading..\n";
            LoadStrings();
        }
    }

    void GameStats::LoadPkmn()
    {
        AnalyzeGameDir();
        _LoadGameStrings();
        _LoadPokemonAndMvData();
    }

    void GameStats::LoadPkmn( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        LoadPkmn();
    }

    void GameStats::_LoadPokemonAndMvData()
    {
        using namespace ::filetypes;
        stringstream sstrMd;
        sstrMd << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory << "/" << MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;
        stringstream sstrGrowth;
        sstrGrowth << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory << "/"  << MLevel_FName;

        cout << "Loading Pokemon Data..\n";

        //Load all the move and move set data at the same time first
        cout << " <*>-Loading move data and Pokemon movesets..\n";
        auto allmovedat = ParseMoveAndLearnsets(sstrMovedat.str());

        //Build all pokemon entries
        cout << " <*>-Building Pokemon database..\n";
        m_pokemonStats = PokemonDB::BuildDB( ParsePokemonBaseData(sstrMd.str()),
                                             std::move(allmovedat.second),
                                             ParseLevelGrowthData(sstrGrowth.str()) );

        //Set move data
        m_moveData1 = std::move(allmovedat.first.first);
        m_moveData2 = std::move(allmovedat.first.second);
        cout << "Done!\n";
    }

    void GameStats::LoadMoves( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        LoadMoves();
    }

    void GameStats::LoadMoves()
    {
        using namespace filetypes;

        AnalyzeGameDir();

        if( m_gameVersion != eGameVersion::Invalid )
        {
            _LoadGameStrings();
            stringstream sstrMovedat;
            sstrMovedat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;

            cout << "Loading moves data..";
            auto allmovedat = ParseMoveAndLearnsets(sstrMovedat.str());

            //Set move data
            m_moveData1 = std::move(allmovedat.first.first);
            m_moveData2 = std::move(allmovedat.first.second);

            cout << " Done!\n";
        }
        else
            throw std::runtime_error( "ERROR: Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void GameStats::LoadItems( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        LoadItems();
    }

    void GameStats::LoadItems()
    {
        using namespace filetypes;
        AnalyzeGameDir();

        if( m_gameVersion != eGameVersion::Invalid )
        {
            _LoadGameStrings();
            _LoadItemData();
            cout << " Done!\n";
        }
        else
            throw std::runtime_error( "Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void GameStats::_LoadItemData()
    {

        //stringstream sstr;
        //sstr << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;
        //m_itemsData = filetypes::ParseItemsData( sstr.str() );
        stringstream sstrItemDat;
        sstrItemDat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;

        cout << "Loading items data..\n";

        if( m_gameVersion == eGameVersion::EoS )
            m_itemsData = std::move( ParseItemsDataEoS( sstrItemDat.str() ) );
        else if( m_gameVersion == eGameVersion::EoTEoD )
            m_itemsData = std::move( ParseItemsDataEoTD( sstrItemDat.str() ) );
        else
            throw runtime_error("GameStats::LoadItems(): Unknown game version!");
    }

    void GameStats::_LoadDungeonData()
    {
        throw exception("Not Implemented!"); //Not implemented yet !
    }

//--------------------------------------------------------------
//  Writing
//--------------------------------------------------------------
    void GameStats::Write()
    {
        //First identify what we're dealing with
        AnalyzeGameDir();
        _WritePokemonAndMvData();
        _WriteItemData();
        _WriteGameStrings();
    }

    void GameStats::Write( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        Write();
    }

    void GameStats::WritePkmn()
    {
        using namespace filetypes;
        //First identify what we're dealing with
        AnalyzeGameDir();
        _WritePokemonAndMvData();
        _WriteGameStrings();
    }

    void GameStats::WritePkmn( const std::string & rootdatafolder )
    {
        using namespace filetypes;
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        WritePkmn();
    }

    void GameStats::_WritePokemonAndMvData()
    {
        using namespace ::filetypes;

        stringstream sstrMd;
        sstrMd << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory << "/" << MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;
        stringstream sstrGrowth;
        sstrGrowth << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory << "/"  << MLevel_FName;
        stringstream sstrstrings;
        sstrstrings << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;

        const string fStatsGrowth = sstrGrowth.str();
        const string fPokeData    = sstrMd.str();
        const string fMoveData    = sstrMovedat.str();
        const string fStrings     = sstrstrings.str();

        cout << "Writing Pokemon Data..\n";

        //Split the pokemon data into its 3 parts
        vector<PokeMonsterData> md;
        pokeMvSets_t            mvset;
        vector<PokeStatsGrowth> sgrowth;
        cout << " <*>-Building move, Pokemon data, and level-up data lists..\n";
        m_pokemonStats.ExportComponents( md, mvset, sgrowth );

        //Write stats growth
        cout << " <*>-Writing Pokemon stats growth file \"" <<fStatsGrowth <<"\"..\n";
        WriteLevelGrowthData( sgrowth, fStatsGrowth );

        //Given the waza file contains both moves and learnsets, we have to load the move data and rewrite it as we modify the pokemon
        // movesets!
        cout << " <*>-Writing move data file(s) to directory \"" <<fMoveData <<"\"..\n";
        if( m_moveData1.empty() )
        {
            cout << "  <!>-Loading move data, as MoveDB had not been loaded..\n";
            auto movedat = ParseMoveData(fMoveData);
            cout << "  <!>-Load complete!\n";
            filetypes::WriteMoveAndLearnsets( fMoveData, movedat, mvset );
        }
        else
        {
            filetypes::WriteMoveAndLearnsets( fMoveData, make_pair( m_moveData1, m_moveData2 ), mvset );
        }

        //Write monster data
        cout << " <*>-Writing Pokemon data file \"" <<fPokeData <<"\"..\n";
        WritePokemonBaseData( md, fPokeData );

        cout << "Done exporting Pokemon data!\n";
    }

    void GameStats::WriteMoves()
    {
        using namespace filetypes;

        AnalyzeGameDir();

        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;
        stringstream sstrstrings;
        sstrstrings << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;

        const string fMoveData    = sstrMovedat.str();
        const string fStrings     = sstrstrings.str();

        cout << "Writing moves Data..\n";

        //Given the waza file contains both moves and learnsets, we have to load the move data and rewrite it as we modify the pokemon
        // movesets!
        cout << " <*>-Need to load partially Pokemon moveset data file(s) from directory \"" <<fMoveData <<"\"..\n";
        auto mvset  = ParsePokemonLearnSets(fMoveData);

        cout << " <*>-Writing data to \"" <<fMoveData <<"\"..\n";
        WriteMoveAndLearnsets( fMoveData, make_pair( MoveDB(m_moveData1), MoveDB(m_moveData2) ), mvset );

        cout << " <*>-Writing game strings to \"" <<fStrings <<"\"..\n";
        _WriteGameStrings();

        cout << "Done writing moves data!\n";
    }

    void GameStats::WriteMoves( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        WriteMoves();
    }

    void GameStats::WriteStrings()
    {
        using namespace filetypes;
        auto prevGameVer = m_gameVersion;

        //Identify target game if we have no info
        AnalyzeGameDir();

        //Warn about game mismatch
        if( prevGameVer != eGameVersion::Invalid && prevGameVer != m_gameVersion )
        {
            clog <<"WARNING: Game version mismatch. The target game data directory is from a different game than the current text data!\n"
                 <<"This will result in unforceen consequences. Continuing..\n";
        }

        _WriteGameStrings();

    }

    void GameStats::WriteStrings( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        WriteStrings();
    }

    void GameStats::_WriteGameStrings()
    {
        using namespace filetypes;
        stringstream sstr;
        sstr << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;
        filetypes::WriteTextStrFile( sstr.str(), m_gameStrings, std::locale( m_gameLangLocale ) );
    }
    
    void GameStats::WriteItems()
    {
        using namespace filetypes;
        auto prevGameVer = m_gameVersion;

        //Identify target game if we have no info
        AnalyzeGameDir();

        _EnsureStringsLoaded();

        //Warn about game mismatch
        if( prevGameVer != eGameVersion::Invalid && prevGameVer != m_gameVersion )
        {
            clog <<"WARNING: Game version mismatch. The target game data directory is from a different game than the current item data!\n"
                 <<"This will result in unforceen consequences! Continuing..\n";
        }

        _WriteItemData();
        _WriteGameStrings();
    }

    void GameStats::WriteItems( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            m_dataFolder = rootdatafolder;

        WriteItems();
    }

    void GameStats::_WriteItemData()
    {
        using namespace filetypes;

        stringstream sstrItemdat;
        sstrItemdat << utils::TryAppendSlash(m_dataFolder) << BalanceDirectory;
        const string balancedirpath = sstrItemdat.str();

        cout << " <*>- Writing item data to \"" <<balancedirpath <<"\"..\n";

        if( m_gameVersion == eGameVersion::EoS )
            WriteItemsDataEoS( balancedirpath, m_itemsData );
        else if( m_gameVersion == eGameVersion::EoTEoD )
            WriteItemsDataEoTD( balancedirpath, m_itemsData );
        else
        {
            throw runtime_error( "GameStats::_WriteItemData(): Unsuported game version !" );
        }
    }

    void GameStats::_WriteDungeonData()
    {
        throw exception("Not Implemented!"); //Not implemented yet !
    }


//--------------------------------------------------------------
//  Export/Import
//--------------------------------------------------------------
    void GameStats::ExportPkmn( const std::string & directory )
    {
        //cout<<"-- Exporting all Pokemon data to XML data --\n";
        if( m_gameStrings.empty() || m_pokemonStats.empty() )
            throw runtime_error("ERROR: Tried to export an empty list of Pokemon ! Or with an empty string list!");

        cout<<" <*>- Writing Pokemon XML data..";
        stats::ExportPokemonsToXML( m_pokemonStats, GetPokemonNameBeg(), GetPokemonCatBeg(), directory );
        cout<<" Done!\n";
    }

    void GameStats::ImportPkmn( const std::string & directory )
    {
        //cout<<"-- Importing all Pokemon from XML data --\n";
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        cout<<" <*>- Parsing Pokemon XML data..";
        stats::ImportPokemonsFromXML( directory, m_pokemonStats, GetPokemonNameBeg(), GetPokemonNameEnd(), GetPokemonCatBeg(), GetPokemonCatEnd() );
        cout<<" Done!\n";
    }

    void GameStats::ExportMoves( const std::string & directory )
    {
        //cout<<"-- Exporting all moves data to XML data --\n";
        if( m_gameStrings.empty() || m_moveData1.empty() )
            throw runtime_error("Move list(s) is/are empty. Or the game strings are not loaded!");

        cout << " <*>- Writing moves to XML data.. ";
        stats::ExportMovesToXML( m_moveData1, m_moveData2, GetMoveNamesBeg(), GetMoveDescBeg(), directory );
        cout << " Done!\n";
    }

    void GameStats::ImportMoves( const std::string & directory )
    {
        //cout<<"-- Importing all moves data from XML data --\n";
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or was not determined. Cannot import move data and format it!");

        cout<<" <*>- Parsing moves XML data..";
        stats::ImportMovesFromXML( directory, m_moveData1, m_moveData2, GetMoveNamesBeg(), GetMoveNamesEnd(), GetMoveDescBeg(), GetMoveDescEnd() );
        cout<<" Done!\n";
    }


    void GameStats::ExportStrings( const std::string & file )
    {
        //cout<<"-- Exporting all game strings to text file \"" <<file <<"\" --\n";
        if( m_gameStrings.empty() )
            throw runtime_error( "No string data to export !" );

        cout<<" <*>- Writing game text to \"" <<file <<"\" ..";
        utils::io::WriteTextFileLineByLine( m_gameStrings, file, locale(m_gameLangLocale) );
        cout<<" Done!\n";
    }

    void GameStats::ImportStrings( const std::string & file )
    {
        //cout<<"-- Importing all game strings from text file --\n";
        //if( m_gameStrings.empty() )
        //    throw runtime_error( "No string data to export !" );

        cout<<" <*>- Importing game text from \"" <<file <<"\" ..";
        m_gameStrings = utils::io::ReadTextFileLineByLine( file, locale(m_gameLangLocale) );
        cout<<"Done importing strings!\n";
    }

    void GameStats::ExportItems( const std::string & directory )
    {
        if( m_gameStrings.empty() ||  m_itemsData.empty() )
            throw runtime_error( "No item data to export, or the strings weren't loaded!!" );

        cout<<" <*>- Exporting items to XML..";
        stats::ExportItemsToXML( m_itemsData, 
                                GetItemNamesBeg(),
                                GetItemNamesEnd(),
                                GetItemShortDescBeg(),
                                GetItemShortDescEnd(), 
                                GetItemLongDescBeg(),
                                GetItemLongDescEnd(),
                                directory );
        cout << " Done!\n";
    }

    void GameStats::ImportItems( const std::string & directory )
    {
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or was not determined. Cannot import item data and format it!");

        cout<<" <*>- Parsing items XML data..";
        m_itemsData = std::move( stats::ImportItemsFromXML( directory, 
                                                           GetItemNamesBeg(),
                                                           GetItemNamesEnd(),
                                                           GetItemShortDescBeg(),
                                                           GetItemShortDescEnd(), 
                                                           GetItemLongDescBeg(),
                                                           GetItemLongDescEnd() ) );
        cout<<" Done!\n";
    }

    void GameStats::ExportAll( const std::string & directory )
    {
        cout<<"-- Exporting everything to XML --\n";
        const string pkmndir = Poco::Path(directory).makeAbsolute().append(DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).makeAbsolute().append(DefMvDir   ).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).makeAbsolute().append(DefItemsDir).makeDirectory().toString();

        if( m_pokemonStats.empty() && m_moveData1.empty() && m_itemsData.empty() )
            throw runtime_error( "No data to export!" );

        _EnsureStringsLoaded();

        if( !m_pokemonStats.empty() )
        {
            utils::DoCreateDirectory(pkmndir);

            cout<<" <*>- Has Pokemon data to export. Exporting to \"" <<pkmndir <<"\"..\n  ";
            ExportPkmn( pkmndir );
        }
        else
            cout<<" <!>- No item Pokemon data to export, skipping..\n";

        if( !m_moveData1.empty() )
        {
            utils::DoCreateDirectory(mvdir);

            cout<<" <*>- Has moves data to export. Exporting to \"" <<mvdir <<"\"..\n  ";
            ExportMoves( mvdir );
        }
        else
            cout<<" <!>- No move data to export, skipping..\n";

        if( !m_itemsData.empty() )
        {
            utils::DoCreateDirectory(itemdir);

            cout<<" <*>- Has items data to export. Exporting to \"" <<itemdir <<"\"..\n  ";
            ExportItems( itemdir );
        }
        else
            cout<<" <!>- No item data to export, skipping..\n";

        cout<<"-- Export complete! --\n";
    }

    void GameStats::ImportAll( const std::string & directory )
    {
        using namespace utils;
        cout<<"-- Importing everything from XML --\n";
        const string pkmndir = Poco::Path(directory).makeAbsolute().append(DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).makeAbsolute().append(DefMvDir   ).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).makeAbsolute().append(DefItemsDir).makeDirectory().toString();

        //Check what we can import
        bool importPokes = pathExists(pkmndir);
        bool importMoves = pathExists(mvdir  );
        bool importItems = pathExists(itemdir);

        if( !importPokes && !importMoves && !importItems )
        {
            stringstream sstrerr;
            sstrerr << "Couldn't find any expected data directories in specified directory!:\n"
                    << " - Expected \"" <<pkmndir <<"\", but no such directory exists!\n" 
                    << " - Expected \"" <<mvdir <<"\", but no such directory exists!\n" 
                    << " - Expected \"" <<itemdir <<"\", but no such directory exists!\n";
            const string strerr = sstrerr.str();
            clog <<strerr <<"\n";
            throw runtime_error( strerr );
        }

        if( !m_pokemonStats.empty() || !m_moveData1.empty() || !m_itemsData.empty() )
            cout <<"  <!>- WARNING: The data already loaded will be overwritten! Continuing happily..\n";

        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        //Check after loading strings
        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or could not be determined!");

        //Run all the import methods
        if(importPokes)
            ImportPkmn(pkmndir);
        if(importMoves)
            ImportMoves(mvdir);
        if(importItems)
            ImportItems(itemdir);

        cout<<"-- Import complete! --\n";
    }

//--------------------------------------------------------------
//  Text Strings Access
//--------------------------------------------------------------

    std::vector<std::string>::const_iterator GameStats::GetPokemonNameBeg()const
    {
        return const_cast<GameStats*>(this)->GetPokemonNameBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetPokemonNameEnd()const
    {
        return const_cast<GameStats*>(this)->GetPokemonNameEnd();
    }

    std::vector<std::string>::iterator GameStats::GetPokemonNameBeg()
    {
        return (m_gameStrings.begin() + strBounds(eStrBNames::PkmnNames).beg );
    }

    std::vector<std::string>::iterator GameStats::GetPokemonNameEnd()
    {
        return (m_gameStrings.begin() + strBounds(eStrBNames::PkmnNames).end );
    }

    std::vector<std::string>::const_iterator GameStats::GetPokemonCatBeg()const
    {
        return const_cast<GameStats*>(this)->GetPokemonCatBeg();
    }
    std::vector<std::string>::const_iterator GameStats::GetPokemonCatEnd()const
    {
        return const_cast<GameStats*>(this)->GetPokemonCatEnd();
    }
    std::vector<std::string>::iterator GameStats::GetPokemonCatBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PkmnCats).beg );
    }
    std::vector<std::string>::iterator GameStats::GetPokemonCatEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PkmnCats).end );
    }


    std::vector<std::string>::const_iterator GameStats::GetMoveNamesBeg()const
    {
        return const_cast<GameStats*>(this)->GetMoveNamesBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetMoveNamesEnd()const
    {
        return const_cast<GameStats*>(this)->GetMoveNamesEnd();
    }

    std::vector<std::string>::iterator       GameStats::GetMoveNamesBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::MvNames).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetMoveNamesEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::MvNames).end );
    }

    std::vector<std::string>::const_iterator GameStats::GetMoveDescBeg()const
    {
        return const_cast<GameStats*>(this)->GetMoveDescBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetMoveDescEnd()const
    {
        return const_cast<GameStats*>(this)->GetMoveDescEnd();
    }

    std::vector<std::string>::iterator       GameStats::GetMoveDescBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::MvDesc).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetMoveDescEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::MvDesc).end );
    }

    std::vector<std::string>::const_iterator GameStats::GetItemNamesBeg()const
    {
        return const_cast<GameStats*>(this)->GetItemNamesBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetItemNamesEnd()const
    {
        return const_cast<GameStats*>(this)->GetItemNamesEnd();
    }

    std::vector<std::string>::iterator       GameStats::GetItemNamesBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemNames).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetItemNamesEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemNames).end );
    }

    std::vector<std::string>::const_iterator GameStats::GetItemShortDescBeg()const
    {
        return const_cast<GameStats*>(this)->GetItemShortDescBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetItemShortDescEnd()const
    {
        return const_cast<GameStats*>(this)->GetItemShortDescEnd();
    }

    std::vector<std::string>::iterator       GameStats::GetItemShortDescBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemDescS).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetItemShortDescEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemDescS).end );
    }

    std::vector<std::string>::const_iterator GameStats::GetItemLongDescBeg()const
    {
        return const_cast<GameStats*>(this)->GetItemLongDescBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetItemLongDescEnd()const
    {
        return const_cast<GameStats*>(this)->GetItemLongDescEnd();
    }

    //Portraits
    std::vector<std::string>::const_iterator GameStats::GetPortraitNamesBeg()const
    {
        return const_cast<GameStats*>(this)->GetPortraitNamesBeg();
    }

    std::vector<std::string>::const_iterator GameStats::GetPortraitNamesEnd()const
    {
        return const_cast<GameStats*>(this)->GetPortraitNamesEnd();
    }

    std::vector<std::string>::iterator       GameStats::GetPortraitNamesBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PortraitNames).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetPortraitNamesEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PortraitNames).end );
    }

    std::vector<std::string>::iterator       GameStats::GetItemLongDescBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemDescL).beg );
    }

    std::vector<std::string>::iterator       GameStats::GetItemLongDescEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::ItemDescL).end );
    }

    std::string & GameStats::GetPokemonNameStr( uint16_t pkmnindex )
    {
        return m_gameStrings[strBounds(eStrBNames::PkmnNames).beg + pkmnindex];
    }

    std::string & GameStats::GetPkmnCatNameStr( uint16_t pkmnindex )
    {
        return m_gameStrings[strBounds(eStrBNames::PkmnCats).beg + pkmnindex];
    }

    std::string & GameStats::GetMoveNameStr( uint16_t moveindex )
    {
        return m_gameStrings[strBounds(eStrBNames::MvNames).beg + moveindex];
    }

    std::string & GameStats::GetMoveDexcStr( uint16_t moveindex )
    {
        return m_gameStrings[strBounds(eStrBNames::MvDesc).beg + moveindex];
    }

    std::string & GameStats::GetAbilityNameStr( uint8_t abilityindex )
    {
        return m_gameStrings[strBounds(eStrBNames::AbilityNames).beg + abilityindex];
    }

    std::string & GameStats::GetAbilityDescStr( uint8_t abilityindex )
    {
        return m_gameStrings[strBounds(eStrBNames::AbilityDesc).beg + abilityindex];
    }

    std::string & GameStats::GetTypeNameStr( uint8_t type )
    {
        return m_gameStrings[strBounds(eStrBNames::TypeNames).beg + type];
    }

    std::string & GameStats::GetItemNameStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemNames).beg + itemindex];
    }

    std::string & GameStats::GetItemSDescStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemDescS).beg + itemindex];
    }

    std::string & GameStats::GetItemLDescStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemDescL).beg + itemindex];
    }



//--------------------------------------------------------------
//  Misc
//--------------------------------------------------------------


};};