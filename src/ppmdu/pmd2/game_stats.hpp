#ifndef GAME_STATS_HPP
#define GAME_STATS_HPP 
/*
game_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    This file contains generic resources for handling the PMD2 game data.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/containers/pokemon_stats.hpp>
#include <ppmdu/containers/item_data.hpp>
#include <ppmdu/containers/move_data.hpp>
#include <ppmdu/pmd2/pmd2_langconf.hpp>
#include <string>
#include <vector>
#include <map>

namespace pmd2{ namespace stats
{ 
    //Game data filenames list
    // -- Pokemon --
    extern const std::string PkmnStatsGrowthFile; //Pokemon stats growth per level, and experience requirement
    extern const std::string PkmnStatsFile;       //Pokemon stats + data
    extern const std::string PkmnMovesFile;       //Pokemon level-up move list + Moves stats 

    // -- Items --
    extern const std::string ItemsStatsFile;      //
    extern const std::string ExcItmsStatsFile;    //

    //Game data location list, from rom root!
    extern const std::string GameStatsFolderPath; // "/BALANCE"

//==================================================================================
//  Functions
//==================================================================================

    /*
        Returns whether the directory contains at least one of
        the expected folders for an importall operation!
    */
    bool isImportAllDir( const std::string & directory );

//==================================================================================
//  Classes
//==================================================================================


    /************************************************************************
        GameStats
            This loads all stats from the PMD2 games into itself.
            It allows to import and export the data it has loaded.
    ************************************************************************/
    class GameStats
    {
        struct strbounds_t
        {
            uint32_t beg = 0;
            uint32_t end = 0;
        };

    public:

        static const std::string DefPkmnDir;
        static const std::string DefMvDir;
        static const std::string DefStrFName;
        static const std::string DefItemsDir;
        static const std::string DefDungeonDir;

        /*
            Pass the game language loader that contains all the known locale strings depending on the game's text_*.str file name.
        */
        //GameStats( const std::string & pmd2rootdir, GameLanguageLoader && langList );
        GameStats( const std::string & pmd2rootdir, const std::string & gamelangfile );

        //Accessors Pokemon Data
        inline const PokemonDB & Pkmn()const                        { return m_pokemonStats; }
        inline PokemonDB       & Pkmn()                             { return m_pokemonStats; }
        inline void              Pkmn( PokemonDB       && newdata ) { m_pokemonStats = newdata; }
        inline void              Pkmn( const PokemonDB &  newdata ) { m_pokemonStats = newdata; }

        inline const std::vector<std::string> Strings()const        { return m_gameStrings; }
        inline       std::vector<std::string> Strings()             { return m_gameStrings; }

        //Accessors 
        const ItemsDB       & Items()const                      { return m_itemsData;    }
        ItemsDB             & Items()                           { return m_itemsData;    }
        void                  Items( ItemsDB       && newdata ) { m_itemsData = newdata; }
        void                  Items( const ItemsDB &  newdata ) { m_itemsData = newdata; }

        //Accessors
        inline void                setCurDataDir( const std::string & path ) { m_dataFolder = path; }
        inline const std::string & getCurDataDir()const                      { return m_dataFolder; }

        //Accessors

        //Input / Output
        /*
            If no path is specified, will use the last path used in either the constructor or in the function below
        */
        void Load       ();
        void LoadStrings();
        void LoadPkmn   ();
        void LoadMoves  ();
        void LoadItems  ();
        void Load       ( const std::string & rootdatafolder );
        void LoadStrings( const std::string & rootdatafolder );
        void LoadPkmn   ( const std::string & rootdatafolder );
        void LoadMoves  ( const std::string & rootdatafolder );
        void LoadItems  ( const std::string & rootdatafolder );
        
        void Write       ();
        void WritePkmn   ();
        void WriteMoves  ();
        void WriteStrings();
        void WriteItems  ();
        void Write       ( const std::string & rootdatafolder );
        void WritePkmn   ( const std::string & rootdatafolder );
        void WriteMoves  ( const std::string & rootdatafolder );
        void WriteStrings( const std::string & rootdatafolder );
        void WriteItems  ( const std::string & rootdatafolder );

        //Export
        /*
            The Current data directory must be set to the game data
            folder the data is exported from !
            Unless everything was loaded!
        */
        void ExportAll    ( const std::string & directory );
        void ExportPkmn   ( const std::string & directory );
        void ExportMoves  ( const std::string & directory );
        void ExportStrings( const std::string & file );
        void ExportItems  ( const std::string & directory );

        //Import
        /*
            The Current data directory must be set to the game data
            folder where the data will be imported to ! This is
            to allow determining the target game version, nothing will be overwritten!
        */
        void ImportAll    ( const std::string & directory );
        void ImportPkmn   ( const std::string & directory );
        void ImportMoves  ( const std::string & directory );
        void ImportStrings( const std::string & file );
        void ImportItems  ( const std::string & directory );

        /*
            Analyze the current data folder to find out what game, and language it is, 
            and where are the correct strings located at.
        */
        inline void AnalyzeGameDir()
        {
            IdentifyGameVersion();
            IdentifyGameLocaleStr();
            BuildListOfStringOffsets();
            if( m_gameVersion == eGameVersion::Invalid )
                throw std::runtime_error( "Couldn't identify the game's version. Some files might be missing..\n" );
        }
        
    public:
        /*
            Text Strings Access
                Use those to get the correct string depending on the current game version.
        */

        //pokemon
        std::vector<std::string>::const_iterator GetPokemonNameBeg()const;
        std::vector<std::string>::const_iterator GetPokemonNameEnd()const;
        std::vector<std::string>::iterator       GetPokemonNameBeg();
        std::vector<std::string>::iterator       GetPokemonNameEnd();

        std::vector<std::string>::const_iterator GetPokemonCatBeg()const;
        std::vector<std::string>::const_iterator GetPokemonCatEnd()const;
        std::vector<std::string>::iterator       GetPokemonCatBeg();
        std::vector<std::string>::iterator       GetPokemonCatEnd();

        //moves
        std::vector<std::string>::const_iterator GetMoveNamesBeg()const;
        std::vector<std::string>::const_iterator GetMoveNamesEnd()const;
        std::vector<std::string>::iterator       GetMoveNamesBeg();
        std::vector<std::string>::iterator       GetMoveNamesEnd();

        std::vector<std::string>::const_iterator GetMoveDescBeg()const;
        std::vector<std::string>::const_iterator GetMoveDescEnd()const;
        std::vector<std::string>::iterator       GetMoveDescBeg();
        std::vector<std::string>::iterator       GetMoveDescEnd();

        //item
        std::vector<std::string>::const_iterator GetItemNamesBeg()const;
        std::vector<std::string>::const_iterator GetItemNamesEnd()const;
        std::vector<std::string>::iterator       GetItemNamesBeg();
        std::vector<std::string>::iterator       GetItemNamesEnd();

        std::vector<std::string>::const_iterator GetItemShortDescBeg()const;
        std::vector<std::string>::const_iterator GetItemShortDescEnd()const;
        std::vector<std::string>::iterator       GetItemShortDescBeg();
        std::vector<std::string>::iterator       GetItemShortDescEnd();

        std::vector<std::string>::const_iterator GetItemLongDescBeg()const;
        std::vector<std::string>::const_iterator GetItemLongDescEnd()const;
        std::vector<std::string>::iterator       GetItemLongDescBeg();
        std::vector<std::string>::iterator       GetItemLongDescEnd();

        std::vector<std::string>::const_iterator GetPortraitNamesBeg()const;
        std::vector<std::string>::const_iterator GetPortraitNamesEnd()const;
        std::vector<std::string>::iterator       GetPortraitNamesBeg();
        std::vector<std::string>::iterator       GetPortraitNamesEnd();

        std::string              & GetPokemonNameStr( uint16_t pkmnindex );
        inline const std::string & GetPokemonNameStr( uint16_t pkmnindex )const  { return const_cast<GameStats*>(this)->GetPokemonNameStr(pkmnindex); }
        std::string              & GetPkmnCatNameStr( uint16_t pkmnindex );
        inline const std::string & GetPkmnCatNameStr( uint16_t pkmnindex )const  { return const_cast<GameStats*>(this)->GetPkmnCatNameStr(pkmnindex); }

        std::string              & GetMoveNameStr   ( uint16_t moveindex );
        inline const std::string & GetMoveNameStr   ( uint16_t moveindex )const  { return const_cast<GameStats*>(this)->GetMoveNameStr(moveindex); }
        std::string              & GetMoveDexcStr   ( uint16_t moveindex );
        inline const std::string & GetMoveDexcStr   ( uint16_t moveindex )const  { return const_cast<GameStats*>(this)->GetMoveDexcStr(moveindex); }

        std::string              & GetAbilityNameStr( uint8_t abilityindex );
        inline const std::string & GetAbilityNameStr( uint8_t abilityindex )const{ return const_cast<GameStats*>(this)->GetAbilityNameStr(abilityindex); }
        std::string              & GetAbilityDescStr( uint8_t abilityindex );
        inline const std::string & GetAbilityDescStr( uint8_t abilityindex )const{ return const_cast<GameStats*>(this)->GetAbilityDescStr(abilityindex); }

        std::string              & GetTypeNameStr   ( uint8_t type );
        inline const std::string & GetTypeNameStr   ( uint8_t type )const        { return const_cast<GameStats*>(this)->GetTypeNameStr(type); }

        std::string              & GetItemNameStr   ( uint16_t itemindex );
        inline const std::string & GetItemNameStr   ( uint16_t itemindex )const  { return const_cast<GameStats*>(this)->GetItemNameStr(itemindex); }
        std::string              & GetItemSDescStr  ( uint16_t itemindex );      //Short Description
        inline const std::string & GetItemSDescStr  ( uint16_t itemindex )const  { return const_cast<GameStats*>(this)->GetItemSDescStr(itemindex); } //Short Description
        std::string              & GetItemLDescStr  ( uint16_t itemindex );      //Long Description
        inline const std::string & GetItemLDescStr  ( uint16_t itemindex )const  { return const_cast<GameStats*>(this)->GetItemLDescStr(itemindex); } //Long Description
    
    private:


        void IdentifyGameVersion     ();
        void IdentifyGameLocaleStr   ();
        void BuildListOfStringOffsets(); //Make a list of all the offsets to the interesting game strings blocks, using the data from the gamelang file, to avoid searching for everytimes the the getstring methods below is called

        void _LoadGameStrings();
        void _LoadPokemonAndMvData(); //Must be written together!
        //void _LoadMoveData   ();
        void _LoadItemData   ();
        void _LoadDungeonData();

        void _WriteGameStrings();
        void _WritePokemonAndMvData(); //Must be written together!
        //void _WriteMoveData   ();
        void _WriteItemData   ();
        void _WriteDungeonData();

        inline strbounds_t strBounds( eStrBNames what )const
        {
            return m_strOffsets[static_cast<uint32_t>(what)];
        }

        //Call this to do a check whether game strings are loaded, and load them as needed!
        void _EnsureStringsLoaded();

    private:

        std::string        m_dataFolder;
        std::string        m_gamelangfile;
        GameLanguageLoader m_possibleLang;
        eGameVersion       m_gameVersion;
        std::string        m_gameLangLocale;
        std::string        m_gameTextFName;
        


        //See enum eStrBNames for what each index is for !
        std::vector<strbounds_t>    m_strOffsets;

        //Game Text
        std::vector<std::string> m_gameStrings;

        //Pokemon Stats
        PokemonDB           m_pokemonStats;

        //Items
        ItemsDB             m_itemsData;

        //#TODO: Combine those two. The move DB should abstract game specific details!!
        MoveDB              m_moveData1;
        MoveDB              m_moveData2; //For Explorers of Sky only

        //Level Data

        //Quiz Data

        //Mini-Game Data

        //Shop Data
    };

};};

#endif