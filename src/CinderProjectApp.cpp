#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "BaundaryTag.h"
#include "cinder/Rand.h"
#include "cinder/Color.h"
using BT = BaundaryTag;
using namespace ci;
using namespace ci::app;
using namespace std;
const size_t MEMORYBYTE = 641;
class CinderProjectApp : public App
{
    char memory[MEMORYBYTE];
    
    BT* manager = nullptr;
    std::vector<BT*> ptrs;
    float prevElapsedSeconds = 0.0F;
    float createActionSeconds = 0.0F;
    float deleteActionSeconds = 0.0F;
public:
    void setup( ) override;
    void update( ) override;
    void draw( ) override;
};
void CinderProjectApp::setup( )
{
    memset( memory, 0, MEMORYBYTE );
    memory[MEMORYBYTE - 1] = BT::State::END;

    manager = new( memory ) BT( MEMORYBYTE - 1 - BT::HEADERBYTE - BT::FOOTERBYTE,
                                BT::State::BEGIN );

    createActionSeconds = randFloat( 0.2F, 1.0F );
    deleteActionSeconds = randFloat( 0.2F, 1.0F );
}
void CinderProjectApp::update( )
{
    float deltaSec = getElapsedSeconds( ) - prevElapsedSeconds;
    prevElapsedSeconds = getElapsedSeconds( );

    createActionSeconds = std::max( 0.0F, createActionSeconds - deltaSec );
    if ( createActionSeconds == 0.0F )
    {
        createActionSeconds = randFloat( 0.2F, 1.0F );

        auto created = manager->create( randInt( sizeof( ColorA ), 40 ) );
        if ( created )
        {
            ColorA* data = reinterpret_cast<ColorA*>( created->getDataPointer( ) );
            *data = hsvToRgb( vec3( randFloat( ), randFloat( 0.5, 0.8 ), randFloat( 0.5, 0.8 ) ) );

            ptrs.emplace_back( created );
        }
    }
    deleteActionSeconds = std::max( 0.0F, deleteActionSeconds - deltaSec );
    if ( deleteActionSeconds == 0.0F )
    {
        deleteActionSeconds = randFloat( 0.2F, 1.0F );

        if ( !ptrs.empty( ) )
        {
            auto index = randInt( 0, ptrs.size( ) );
            if ( ptrs[index]->IsUse( ) )
            {
                delete ptrs[index];
                ptrs.erase( ptrs.begin( ) + index );
            }
        }
    }
}
void CinderProjectApp::draw( )
{
    gl::color( ColorA::white( ) );
    gl::clear( Color( 0, 0, 0 ) );

    int index = 0;
    while ( true )
    {
        BT* tag = reinterpret_cast<BT*>( memory + index );
        if ( tag->IsEnd( ) ) break;
        if ( tag->IsUse( ) )
        {
            ColorA* data = reinterpret_cast<ColorA*>( tag->getDataPointer( ) );
            gl::color( *data );
            for ( u_int i = index; i < index + tag->getOverallByte( ); ++i )
            {
                gl::drawLine( vec2( i, 0 ), vec2( i, app::getWindowHeight( ) ) );
            }
        }
        u_int overallByte = tag->getOverallByte( );
        if ( overallByte == 0 ) break;
        index += overallByte;
    }
}
CINDER_APP( CinderProjectApp, RendererGl, [ ] ( cinder::app::App::Settings* setting )
{
    setting->setConsoleWindowEnabled( );
} )
