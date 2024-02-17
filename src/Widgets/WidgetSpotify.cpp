#include <ArduinoJson.h>
#include <JPEGDEC.h>

#include "WidgetSpotify.h"

namespace ts {

static const uint8_t bayerThresholdMap [4][4] = {
    {15, 135, 45, 165},
    {195, 75, 225, 105},
    {60, 180, 30, 150},
    {240, 120, 210, 90},
};

static int JPEGDraw(JPEGDRAW *pDraw)
{
    log_i("Drawing image!\n");

    bool dither = true;
    int16_t x = pDraw->x;
    int16_t y = pDraw->y;
    int16_t w = pDraw->iWidth;
    int16_t h = pDraw->iHeight;

    ts::BitmapInterface* pBuffer = (ts::BitmapInterface*)pDraw->pUser;
    uint8_t threshold = 128;

    log_i("Drawing buffer.");

    for(int16_t j = 0; j < h; j++)
    {
        for(int16_t i = 0; i < w; i++)
        {
            pDraw->pPixels[i + j * w] = (pDraw->pPixels[i + j * w] & 0x7e0) >> 5; // extract just the six green channel bits.
        
            // Bayer Dithering
            // 4x4 Bayer ordered dithering algorithm
            const uint16_t bayerx = (i + j * w) % w;
            const uint16_t bayery = floor((i + j * w) / w);
            const uint16_t bayerMapped = 0 + (pDraw->pPixels[i + j * w] - 0) * (255 - 0) / (63 - 0);
            const uint16_t map = floor((bayerMapped + bayerThresholdMap[bayerx % 4][bayery % 4]) / 2);
            const ts::Vector2i pos{x+i, y+j};
            pBuffer->set(pos, (map < threshold) ? false : true);
        }
    }

    

    // 
    // if (dither)
    // {
    //     for(int16_t j = 0; j < h; j++)
    //     {
    //         for(int16_t i = 0; i < w; i++)
    //         {
    //             int8_t oldPixel = constrain(pDraw->pPixels[i + j * w], 0, 0x3F);
    //             int8_t newPixel = oldPixel & 0x30; // or 0x30 to dither to 2-bit directly. much improved tonal range, but more horizontal banding between blocks.
    //             pDraw->pPixels[i + j * w] = newPixel;
    //             int quantError = oldPixel - newPixel;      
    //             if(i + 1 < w) 
    //                 pDraw->pPixels[i + 1 + j * w] += quantError * 7 / 16;
    //             if((i - 1 >= 0) && (j + 1 < h)) 
    //                 pDraw->pPixels[i - 1 + (j + 1) * w] += quantError * 3 / 16;
    //             if(j + 1 < h) 
    //                 pDraw->pPixels[i + (j + 1) * w] += quantError * 5 / 16;
    //             if((i + 1 < w) && (j + 1 < h)) 
    //                 pDraw->pPixels[i + 1 + (j + 1) * w] += quantError * 1 / 16;
    //         } // for i
    //     } // for j
    // } // if dither
  // 
    // ts::BitmapInterface* pBuffer = (ts::BitmapInterface*)pDraw->pUser;
    // ts::Vector2i pos;
    // for(int16_t i = 0; i < w; i++)
    // {
    //     for(int16_t j = 0; j < h; j++)
    //     {
    //         pos = ts::Vector2i{(int16_t)(x+i), (int16_t)(y+j)};
    //         
    //         switch (constrain(pDraw->pPixels[i + j * w] >> 4, 0, 3))
    //         {
    //             
    //             case 0:
    //             pBuffer->set(pos, false);
    //             break;
    //             case 1:
    //             case 2:
    //             case 3:
    //                 pBuffer->set(pos, true);
    //             break;
    //         } // switch
    //     } // for j
    // } // for i
    return 1;
} /* JPEGDraw() */


WidgetSpotify::WidgetSpotify(SpotifyESP& spotify)
    : _spotify(&spotify)
{
}

bool WidgetSpotify::fetch(WiFiClientSecure& client)
{

    /* Spotifys API requires us to generate a token before other requests. */
    log_i("Requesting Spotify currently playing.");
    
    char imageURL[SPOTIFY_URL_CHAR_LENGTH];

    bool isPlaying = false;
    auto onCurrentyPlaying = [&](SpotifyCurrentlyPlaying currentlyPlaying){
        log_i("Track name: %s", currentlyPlaying.trackName);
        log_i("Artist name: %s", currentlyPlaying.artists[0].artistName);

        isPlaying = false;
        _currentlyPlaying = currentlyPlaying.isPlaying ? Strings::ePlaybackNowPlaying : Strings::ePlaybackWasPlaying;
        strncpy(_title, currentlyPlaying.trackName, SPOTIFY_NAME_CHAR_LENGTH);
        strncpy(_artist, currentlyPlaying.artists[0].artistName, SPOTIFY_NAME_CHAR_LENGTH);

        if (currentlyPlaying.isPlaying)
            strncpy(imageURL, currentlyPlaying.albumImages[/* smallest */ 1].url, SPOTIFY_URL_CHAR_LENGTH);
    };

    client.setCACert(SpotifyCert::server);
    _spotify->getCurrentlyPlayingTrack(onCurrentyPlaying, "US");

    yield();

    //if (!isPlaying) {
    //    auto onRecentlyPlayed = [&](SpotifyRecentlyPlayed recentlyPlayed){
    //        log_i("Track name: %s", currentlyPlaying.trackName);
    //        log_i("Artist name: %s", currentlyPlaying.artists[0].artistName);
//
    //        isPlaying = false;
    //        _currentlyPlaying = currentlyPlaying.isPlaying ? Strings::ePlaybackNowPlaying : Strings::ePlaybackWasPlaying;
    //        strncpy(_title, currentlyPlaying.trackName, SPOTIFY_NAME_CHAR_LENGTH);
    //        strncpy(_artist, currentlyPlaying.artists[0].artistName, SPOTIFY_NAME_CHAR_LENGTH);
//
    //        if (currentlyPlaying.isPlaying)
    //            strncpy(imageURL, currentlyPlaying.albumImages[/* smallest */ 1].url, SPOTIFY_URL_CHAR_LENGTH);
    //    };
    // 
    //    _spotify->gaetRecentlyPlayedTracks(onRecentlyPlayed);
    //}

    log_i("Artist: %s", _artist);
    log_i("Track: %s", _title);

#ifdef TALOS_SUPPORT_SPOTIFY_IMAGES
    client.setInsecure();

    log_i("Memory before gathering image: %d", ESP.getFreeHeap());

    if (!_spotify->requestImage(imageURL, &_imageLength)) {
        log_e("Could not request an image!");
        return true;
    }

    _image = (uint8_t*)ps_malloc(_imageLength);
    
    if (!_image) {
        log_e("Could not allocate memory for spotify image, no image this time!");
        return true;
    }

    if (!_spotify->getImage(_image))
        log_e("Could not get a Spotify image, not displaying.");
#endif

    return true;
}

void WidgetSpotify::render(Render& render)
{
    const int16_t titleY = 480/2;
    const int16_t largeFont = 96;
    const int16_t smallFont = 56;


    log_i("Creting a JPEG description!\n");
    JPEGDEC* dec = new JPEGDEC();
    
    log_i("Bitmap: %p\n", render.getBitmap());
    
    dec->setUserPointer(render.getBitmap());
    dec->setPixelType(ONE_BIT_DITHERED);
    
    // dec->open()

    log_i("Opening JPEG, Pointer: %p, Length %d \n", _image, _imageLength);
    if (!dec->openRAM(_image, _imageLength, JPEGDraw))
    {
        log_e("Could not open JPEGDEC from memory!\n");
    }

    //dec->open()    
    
    dec->setUserPointer(render.getBitmap());
    
    log_i("Decoding jpeg image!\n");
    if (!dec->decode(400, 75, 0))
    {
        log_e("Could not decode JPEGDEC image!\n");
    }
    
    log_i("Decoded JPEG image!\n");

    render
        .setAlignment(RenderAlign::eBottomLeft)
        .setCursor(Vector2i{10, (int16_t)(titleY - largeFont/2)}) // 480/2 + 1 - trackSize/2
        .setFontSize(smallFont)
        .drawTextFromFlash(_currentlyPlaying)

        .setFillColor(Color::eBlack)
        .setOutlineColor(Color::eWhite)
        .setFontSize(96)
        .setAlignment(RenderAlign::eMiddleLeft)
        .setCursor(Vector2i{10, titleY})
        .drawText(_title)

        .setAlignment(RenderAlign::eTopLeft)
        .setCursor(Vector2i{10, (int16_t)(titleY + largeFont/2)})
        .drawText(_artist); 

    if (_image)
        free(_image);

}

} /* namespace ts */