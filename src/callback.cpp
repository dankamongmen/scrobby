/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <curl/curl.h>
#include <cstring>

#include "callback.h"
#include "misc.h"
#include "scrobby.h"
#include "song.h"

using std::string;

MPD::State old_state = MPD::psUnknown;
MPD::State current_state = MPD::psUnknown;

extern Handshake handshake;
extern MPD::Song s;

extern pthread_mutex_t curl_lock;
extern pthread_mutex_t handshake_lock;

extern bool notify_about_now_playing;

void ScrobbyErrorCallback(MPD::Connection *, int, string errormessage, void *)
{
	ignore_newlines(errormessage);
	Log(llVerbose, "MPD: %s", errormessage.c_str());
}

void ScrobbyStatusChanged(MPD::Connection *Mpd, MPD::StatusChanges changed, void *)
{
	if (changed.State)
	{
		old_state = current_state;
		current_state = Mpd->GetState();
		if (old_state == MPD::psStop && current_state == MPD::psPlay)
			changed.SongID = 1;
	}
	if (changed.ElapsedTime)
	{
		static int crossfade;
		if (Mpd->GetElapsedTime() == ((crossfade = Mpd->GetCrossfade()) ? crossfade : 0 ))
			changed.SongID = 1;
		s.Playback()++;
	}
	if (changed.SongID || (old_state == MPD::psPlay && current_state == MPD::psStop))
	{
		s.Submit();
		
		// in this case allow entering only once
		if (old_state == MPD::psPlay && current_state == MPD::psStop)
			old_state = MPD::psUnknown;
		
		if (Mpd->GetElapsedTime() < Mpd->GetCrossfade()+5)
			s.SetStartTime();
		
		if (current_state == MPD::psPlay || current_state == MPD::psPause)
		{
			s.SetData(Mpd->CurrentSong());
			notify_about_now_playing = s.Data() && !s.isStream();
		}
	}
	if (notify_about_now_playing)
	{
		pthread_mutex_lock(&handshake_lock);
		if (s.Data() && (!s.Data()->artist || !s.Data()->title))
		{
			Log(llInfo, "Playing song with missing tags detected.");
		}
		else if (s.Data() && s.Data()->time <= 0)
		{
			Log(llInfo, "Playing song with unknown length detected.");
		}
		else if (s.Data() && s.Data()->artist && s.Data()->title)
		{
			Log(llVerbose, "Playing song detected: %s - %s", s.Data()->artist, s.Data()->title);
			
			if (handshake.status == "OK" && !handshake.nowplaying_url.empty())
			{
				Log(llInfo, "Sending now playing notification...");
			}
			else
			{
				Log(llInfo, "Notification not sent due to problem with connection.");
				goto NOTIFICATION_FAILED;
			}
			
			string result, postdata;
			CURLcode code;
			
			pthread_mutex_lock(&curl_lock);
			CURL *np_notification = curl_easy_init();
			
			char *c_artist = curl_easy_escape(np_notification, s.Data()->artist, 0);
			char *c_title = curl_easy_escape(np_notification, s.Data()->title, 0);
			char *c_album = s.Data()->album ? curl_easy_escape(np_notification, s.Data()->album, 0) : NULL;
			char *c_track = s.Data()->track ? curl_easy_escape(np_notification, s.Data()->track, 0) : NULL;
			
			postdata = "s=";
			postdata += handshake.session_id;
			postdata += "&a=";
			postdata += c_artist;
			postdata += "&t=";
			postdata += c_title;
			postdata += "&b=";
			if (c_album)
				postdata += c_album;
			postdata += "&l=";
			postdata += IntoStr(s.Data()->time);
			postdata += "&n=";
			if (c_track)
				postdata += c_track;
			postdata += "&m=";
			
			curl_free(c_artist);
			curl_free(c_title);
			curl_free(c_album);
			curl_free(c_track);
			
			Log(llVerbose, "URL: %s", handshake.nowplaying_url.c_str());
			Log(llVerbose, "Post data: %s", postdata.c_str());
			
			curl_easy_setopt(np_notification, CURLOPT_URL, handshake.nowplaying_url.c_str());
			curl_easy_setopt(np_notification, CURLOPT_POST, 1);
			curl_easy_setopt(np_notification, CURLOPT_POSTFIELDS, postdata.c_str());
			curl_easy_setopt(np_notification, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(np_notification, CURLOPT_WRITEDATA, &result);
			curl_easy_setopt(np_notification, CURLOPT_CONNECTTIMEOUT, curl_timeout);
			code = curl_easy_perform(np_notification);
			curl_easy_cleanup(np_notification);
			pthread_mutex_unlock(&curl_lock);
			
			ignore_newlines(result);
			
			if (result == "OK")
			{
				Log(llInfo, "Notification about currently playing song sent.");
			}
			else
			{
				if (result.empty())
				{
					Log(llInfo, "Error while sending notification: %s", curl_easy_strerror(code));
				}
				else
				{
					Log(llInfo, "Audioscrobbler returned status %s", result.c_str());
				}
				goto NOTIFICATION_FAILED;
			}
		}
		if (0)
		{
			NOTIFICATION_FAILED:
			
			handshake.Clear(); // handshake probably failed if we are here, so reset it
			Log(llVerbose, "Handshake status reset");
		}
		pthread_mutex_unlock(&handshake_lock);
		notify_about_now_playing = 0;
	}
}

