#include "Spotify.h"

namespace ts {



void Spotify::begin(AsyncWebServer& server)
{ 
    _server = &server;

    /* Add spotify callbacks for the authentication process. */
    _server->on("/spotify", login);
    _server->on("/spotify_callback", callback);
}

void Spotify::login(AsyncWebServerRequest* request)
{

    /* Generate a random state value. */
    const int stateLength = 16;
    char state[stateLength+1];
    memset(state, 0, sizeof(state));
    esp_fill_random(state, sizeof(state)-1);

    /* Spotify scope of our TALOS. */
    const __FlashStringHelper* scope = 
      F("user-read-private+"
        "user-read-currently-playing+"
        "user-read-playback-state");

    /* Redirects to the spotify_callback to receive the code. */
    const __FlashStringHelper* redirect = F("http://talos.local/spotify_callback");

    /* Build the Spotify redirect for authorization. */
    String url;
    url.reserve(500);
    url.concat(F("https://accounts.spotify.com/authorize/?response_type=code&scope="));
    url.concat(scope);
    url.concat(F("&redirect_uri="));
    url.concat(redirect);
    url.concat(F("&state="));
    url.concat(state);

    request->redirect(url);
}

void Spotify::callback(AsyncWebServerRequest* request)
{
    AsyncWebParameter* code = request->getParam("code");
    
    String spotifyCode = code->value();

    

}

} /* namespace ts */