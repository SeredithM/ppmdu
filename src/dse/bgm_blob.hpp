#ifndef BGM_BLOB_HPP
#define BGM_BLOB_HPP
/*
bgm_blob.hpp
2015/12/21
psycommando@gmail.com
Description:
    This is used to access a blob, aka unstructured data piled up, of DSE containers like SWDL, SMDL, and SEDL.
    Can also be used on NDS ROMs directly, but only when the data doesn't uses a main bank, unless the main bank name is specified.
*/
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>

namespace DSE
{
    //=========================================================================================
    //  BlobScanner
    //=========================================================================================
    template<class _rndit>
        class BlobScanner
    {
    public:
        typedef _rndit inputiterator;

        struct FoundContainer
        {
            std::string    _name;
            eDSEContainers _type;
            inputiterator  _beg;
            inputiterator  _end;
        };

        static const uint8_t FirstCharacterDSEMagicNum = 0x73;
        static const size_t  OffsetDSECntFilename      = 32;
        static const size_t  FilenameLength            = 16;
        
        BlobScanner( inputiterator beg, inputiterator end )
            :m_srcbeg(beg), m_srcend(end)
        {}

        /*
            Scan
                Finds all DSE containers within a blob of data.
                Store them in a ToC to allow easy retrieval later on.
        */
        size_t Scan( bool bquiet = false )
        {
            using namespace std;
            for( auto itby = m_srcbeg; itby != m_srcend;  )
            {
                if( *itby == FirstCharacterDSEMagicNum )
                {
                    //Try to see if its indeed the beginning of a DSE header!
                    uint32_t fullmagic   = 0;
                    auto     itbefmagicn = itby; //Save our position before the magic number
                    auto     aftermagicn = utils::ReadIntFromBytes<uint32_t, decltype(itby), false>( fullmagic, itby, m_srcend );

                    eDSEContainers cntty = IntToContainerMagicNum( fullmagic );
                    if( cntty != eDSEContainers::invalid )
                    {
                        itby = HandleContainer( aftermagicn, itbefmagicn, cntty );

                        if(!bquiet)
                            cout << "<*>- Found container " <<m_toc.back()._name <<", of type " <<hex <<showbase <<cntty <<noshowbase <<dec <<" !\n";
                    }
                    else
                        ++itby;
                }
                else
                    ++itby;
            }

            return m_toc.size();
        }

        std::vector<std::pair<FoundContainer,FoundContainer>> ListAllMatchingSMDLPairs()
        {
            using namespace std;
            //Ensure the content was scanned at least once
            if( m_toc.empty() )
                Scan();

            vector<pair<FoundContainer, FoundContainer>> matches;
            for( const auto entry : m_toc )
            {
                if( entry._type == eDSEContainers::smdl )
                {
                    auto itfoundswdl = find_if( m_toc.begin(), m_toc.end(), [&]( const BlobScanner<_rndit>::FoundContainer & cn )->bool
                    {
                        if( cn._type == eDSEContainers::swdl )
                        {
                            if( PairNameMatches( entry._name, cn._name ) )
                                return true;
                        }
                        return false;
                    });

                    if( itfoundswdl != m_toc.end() )
                    {
                        matches.push_back( move( make_pair( FoundContainer(*itfoundswdl), FoundContainer(entry) ) ) );
                    }
                }
            }

            return move(matches);
        }

        std::vector<std::pair<FoundContainer,FoundContainer>> ListAllMatchingSEDLPairs()
        {
            using namespace std;
            //Ensure the content was scanned at least once
            if( m_toc.empty() )
                Scan();

            assert(false);
            return std::vector<std::pair<FoundContainer,FoundContainer>>();
        }

        std::vector<FoundContainer> ListAllSADL()
        {
            using namespace std;
            //Ensure the content was scanned at least once
            if( m_toc.empty() )
                Scan();

            assert(false);
            return std::vector<FoundContainer>();
        }

        /*
            FindSWDL
                Returns any SWDLs with a matching internal name!

        */
        std::vector<FoundContainer> FindSWDL( const std::string & swdlinternalname )
        {
            using namespace std;
            //Ensure the content was scanned at least once
            if( m_toc.empty() )
                Scan();

            std::vector<FoundContainer> matches;

            for( const auto entry : m_toc )
            {
                if( entry._name == swdlinternalname )
                    matches.push_back(entry);
            }

            return move(matches);
        }

    private:

        inputiterator HandleContainer( inputiterator itaftermagicnum, inputiterator itbefmagicnum, eDSEContainers cnty )
        {
            using namespace std;
            //All DSE containers have a filesize 4 bytes after their magic number
            std::advance( itaftermagicnum, 4 );
            uint32_t flen = 0;
            itaftermagicnum = utils::ReadIntFromBytes<uint32_t, inputiterator, true>( flen, itaftermagicnum, m_srcend );

            //Readfilename
            inputiterator itname = itbefmagicnum;
            std::advance( itname, OffsetDSECntFilename );
            inputiterator itnameed = itname;
            std::advance( itnameed, FilenameLength );

            //Find the end, and add the entry to the ToC!
            inputiterator itend = utils::advAsMuchAsPossible( itbefmagicnum, m_srcend, flen );
            m_toc.push_back( FoundContainer{ string(itname, itnameed), cnty, itbefmagicnum, itend } );
            return itend;
        }

        static bool PairNameMatches( const std::string & name1, const std::string & name2 )
        {
            using namespace std;

            if( name1.size() != name2.size() )
                return false;

            size_t posdot = name1.find_last_of('.');

            if( posdot != string::npos )
                return name1.compare( 0, posdot+1, name2, 0, posdot+1 ) == 0;
            else
                return name1.compare( name2 ) == 0;
        }

    private:
        inputiterator               m_srcbeg;
        inputiterator               m_srcend;
        std::vector<FoundContainer> m_toc;
    };
};

#endif