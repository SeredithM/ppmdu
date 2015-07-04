#include "pmd2_audio_data.hpp"
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <sstream>
#include <iomanip>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{
    class TrackPlaybackState
    {
    public:

        TrackPlaybackState()
            :curpitch(0), curbpm(0), lastsilence(0), curvol(0), curexp(0), curpreset(0), curpan(0),lastpitchev(0)
        {}

        std::string printevent( const TrkEvent & ev )
        {
            stringstream outstr;
        
            //Print Delta Time
            if( ev.dt != 0 )
            {
                outstr <<dec <<static_cast<uint16_t>( DSE::TrkDelayCodeVals.at(ev.dt) )  <<" ticks-";
            }

            auto evinf = GetEventInfo( static_cast<eTrkEventCodes>(ev.evcode) );

            //Print Event Label
            if( evinf.first )
                outstr << evinf.second.evlbl << "-";
            else
                outstr << "INVALID-";

            //Print Parameters and Event Specifics
            if( evinf.second.nbreqparams == 1 )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg )
                {
                    outstr << "( vel:" <<dec << static_cast<unsigned short>(ev.evcode) <<", TrkPitch:";
                    uint8_t prevpitch = curpitch;
                    uint8_t pitchop   = (NoteEvParam1PitchMask & ev.params.front());

                    if( pitchop == static_cast<uint8_t>(eNotePitch::lower) ) 
                        curpitch -= 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::higher) ) 
                        curpitch += 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::reset) ) 
                        curpitch = lastpitchev;

                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch);
                    
                    outstr <<", note: " <<DSE::NoteNames.at( (ev.params.front() & NoteEvParam1NoteMask) );
                    
                    //Ugly but just for debug...
                    //switch( static_cast<eNote>(ev.param1 & NoteEvParam1NoteMask) )
                    //{

                    //    case eNote::C:  outstr <<"C";  break;
                    //    case eNote::Cs: outstr <<"C#"; break;
                    //    case eNote::D:  outstr <<"D";  break;
                    //    case eNote::Ds: outstr <<"D#"; break;
                    //    case eNote::E:  outstr <<"E";  break;
                    //    case eNote::F:  outstr <<"F";  break;
                    //    case eNote::Fs: outstr <<"F#"; break;
                    //    case eNote::G:  outstr <<"G";  break;
                    //    case eNote::Gs: outstr <<"G#";  break;
                    //    case eNote::A:  outstr <<"A";  break;
                    //    case eNote::As: outstr <<"A#"; break;
                    //    case eNote::B:  outstr <<"B";  break;
                    //};
                    outstr <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetOctave )
                {
                    outstr <<"( TrkPitch: ";
                    uint8_t prevpitch = curpitch;
                    lastpitchev = ev.params.front();
                    curpitch    = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetExpress )
                {
                    outstr <<"( TrkExp: ";
                    int8_t prevexp = curexp;
                    curexp = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevexp) <<"->" <<dec <<static_cast<short>(curexp) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkVol )
                {
                    outstr <<"( Vol: ";
                    int8_t prevvol = curvol;
                    curvol = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevvol) <<"->" <<dec <<static_cast<short>(curvol) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkPan )
                {
                    outstr <<"( pan: ";
                    int8_t prevpan = curpan;
                    curpan = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpan) <<"->" <<dec <<static_cast<short>(curpan) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetPreset )
                {
                    outstr <<"( Prgm: ";
                    uint8_t prevpreset = curpreset;
                    curpreset = ev.params.front();
                    outstr <<dec <<static_cast<unsigned short>(prevpreset) <<"->" <<dec <<static_cast<unsigned short>(curpreset) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTempo )
                {
                    outstr <<"( tempo: ";
                    int8_t prevbpm = curbpm;
                    curbpm = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevbpm) <<"->" <<dec <<static_cast<short>(curbpm) <<" )";
                }
                else
                    outstr << "( param1: 0x" <<hex <<static_cast<unsigned short>(ev.params.front()) <<" )" <<dec;
            }

            //Print Event with 2 params or a int16 as param
            if( ( evinf.second.nbreqparams == 2 ) )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::LongPause )
                {
                    uint16_t duration = ( static_cast<uint16_t>(ev.params[1] << 8) | static_cast<uint16_t>(ev.params.front()) );
                    outstr << "( duration: 0x" <<hex <<duration <<" )" <<dec;
                }
                else
                    outstr << "( param1: 0x"  <<hex <<static_cast<unsigned short>(ev.params[0]) <<" , param2: 0x" <<static_cast<unsigned short>(ev.params[1]) <<" )" <<dec;
            }

            //if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg  )
            if( ev.params.size() > 2 )
            {
                //const uint8_t nbextraparams = (ev.params.front() & NoteEvParam1NbParamsMask) >> 6; // Get the two highest bits (1100 0000)

                outstr << "( ";

                for( unsigned int i = 0; i < ev.params.size(); ++i )
                {
                    outstr << "param" <<dec <<i <<": 0x" <<hex <<static_cast<unsigned short>(ev.params[i]) <<dec;

                    if( i != (ev.params.size()-1) )
                        outstr << ",";
                    else
                        outstr <<" )";
                }
                
            }

            return outstr.str();
        }

    private:
        //ittrk_t m_beg;
        //ittrk_t m_loop;
        //ittrk_t m_cur;
        //ittrk_t m_end;

        int8_t   curvol;
        int8_t   curpan;
        int8_t   curexp;
        uint8_t  curpitch;
        uint8_t  lastpitchev;
        uint8_t  curbpm;
        uint8_t  curpreset;
        uint16_t lastsilence;
    };

    std::string MusicSequence::tostr()const
    {
        stringstream sstr;
        int cnt = 0;
        for( const auto & track : m_tracks )
        {
            sstr <<"==== Track" <<cnt << " ====\n\n";
            TrackPlaybackState st;
            for( const auto & ev : track )
            {
                sstr << st.printevent( ev ) << "\n";
            }
            ++cnt;
        }

        return move(sstr.str());
    }

    std::string MusicSequence::printinfo()const
    {
        stringstream sstr;
        sstr << " ==== " <<m_meta.fname <<" ==== \n"
             << "CREATE ITME : " <<m_meta.createtime <<"\n"
             << "NB TRACKS   : " <<m_tracks.size()   <<"\n"
             << "TPQN        : " <<m_meta.tpqn       <<"\n";

        return sstr.str();
    }

//
//  BatchAudioLoader
//

    BatchAudioLoader::BatchAudioLoader( const std::string & mbank )
        :m_mbankpath(mbank)
    {
    }
    
    void BatchAudioLoader::LoadMasterBank()
    {
        m_master = move( LoadSwdBank( m_mbankpath ) );
    }

    void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    {
        m_mbankpath = mbank;
        LoadMasterBank();
    }

    void BatchAudioLoader::LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        m_pairs.push_back( move( pmd2::audio::LoadSmdSwdPair( smd, swd ) ) );
    }

    pair< vector< vector<InstrumentInfo*> >, 
          vector< map<uint16_t,size_t> > >   BatchAudioLoader::PrepareMergedInstrumentTable()const
    {
        std::vector< std::vector<InstrumentInfo*> > merged( m_master.metadata().nbprgislots );
        
        //List of what slot the instruments were put into for each SMD+SWD pair
        std::vector< std::map<uint16_t,std::size_t> > smdlPresLocation (m_pairs.size()); 

        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            const auto & apair      = m_pairs[cntpair];
            auto         ptrinstinf = apair.second.instbank().lock();
            
            if( ptrinstinf != nullptr )
            {
                const auto & curinstlist = ptrinstinf->instinfo();

                for( size_t cntinst = 0; cntinst < curinstlist.size();  ++cntinst ) //Test all the individual instruments and add them to their slot
                {
                    //Check if we have a duplicate
                    if( curinstlist[cntinst] != nullptr )
                    {
                        auto founddup = find_if( merged[cntinst].begin(), 
                                                 merged[cntinst].end(), 
                                                 [&]( const InstrumentInfo * inf )->bool
                        { 
                            if( inf != nullptr )
                                return (inf->isSimilar( *(curinstlist[cntinst].get()) ) != InstrumentInfo::eCompareRes::different );
                            else
                            {
                                throw std::exception("BatchAudioLoader::PrepareMergedInstrumentTable(): Null instrument pointer?!");
                                return false;
                            }
                        });

                        if( founddup != merged[cntinst].end() )
                        {
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, distance( merged[cntinst].begin(), founddup) ) );
                        }
                        else
                        {
                            //Push if nothing similar found
                            merged[cntinst].push_back( curinstlist[cntinst].get() );
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, merged[cntinst].size()-1 ) );
                        }
                    }
                    //Don't mark the position of null entries
                }
            }
        }

        return make_pair( std::move(merged), std::move(smdlPresLocation) );
    }

    void BatchAudioLoader::ExportSoundfont( const std::string & destf )const
    {
        //First build a master instrument list from all our pairs, with multiple entries per instruments
        auto merged = PrepareMergedInstrumentTable();

        //#TODO: Find a way to pass elegantly the list of all the positions of the instrument presets from the smd+swd pairs in the merged instrument list !


        //Then send that to the Soundfont writing function.

        //Write the soundfont
    }

    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir )const
    {

    }

    void BatchAudioLoader::ExportSoundfontAsGM( const std::string                               & destf, 
                                                const std::map< std::string, std::vector<int> > & dsetogm )const
    {
    }


//===========================================================================================
//  Functions
//===========================================================================================
    
    std::pair< PresetBank, std::vector<std::pair<MusicSequence,PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot )
    {
        PresetBank                                  mbank  = DSE::ParseSWDL( bank );
        vector<std::pair<MusicSequence,PresetBank>> seqpairs;

        //We can't know for sure if they're always in the same directory!
        //vector<string> smdfnames;
        //vector<string> swdfnames;
        Poco::DirectoryIterator dirend;
        Poco::File              dirswd( swdroot );
        Poco::File              dirsmd( smdroot );
        vector<Poco::File>      cntdirswd;
        vector<Poco::File>      cntdirsmd;

        //Fill up file lists
        dirswd.list(cntdirswd);
        dirsmd.list(cntdirsmd);
        
        //Find and load all smd/swd pairs!
        for( const auto & smd : cntdirsmd )
        {
            const string smdbasename = Poco::Path(smd.path()).getBaseName();

            //Find matching swd
            vector<Poco::File>::const_iterator itfound = cntdirswd.begin();
            for( ; itfound != cntdirswd.end(); ++itfound )
            {
                if( smdbasename == Poco::Path(itfound->path()).getBaseName() )
                    break;
            }

            if( itfound == cntdirswd.end() )
            {
                cerr<<"Skipping " <<smdbasename <<".smd, because its corresponding " <<smdbasename <<".swd can't be found!\n";
                continue;
            }

            //Parse the files, and push_back the pair
            seqpairs.push_back( make_pair( DSE::ParseSMDL( smd.path() ), DSE::ParseSWDL( itfound->path() ) ) );
        }

        return make_pair( std::move(mbank), std::move(seqpairs) );
    }

    std::pair<MusicSequence,PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        PresetBank    bank = DSE::ParseSWDL( swd );
        MusicSequence seq  = DSE::ParseSMDL( smd );
        return std::make_pair( std::move(seq), std::move(bank) );
    }

    PresetBank LoadSwdBank( const std::string & file )
    {
        return DSE::ParseSWDL( file );
    }
        
    MusicSequence LoadSequence( const std::string & file )
    {
        return DSE::ParseSMDL( file );
    }


};};