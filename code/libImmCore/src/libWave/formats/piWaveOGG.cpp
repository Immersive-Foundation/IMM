#include <malloc.h>
#include <string.h>

#include "../piWave.h"
#include "../../libBasics/piTArray.h"
#include "../../libBasics/piStreamI.h"
#include "../../libBasics/piStreamFileI.h"
#include "../../libBasics/piStreamArrayI.h"
#include "../../libBasics/piStreamO.h"
#include "../../libBasics/piStreamFileO.h"
#include "../../libBasics/piStreamArrayO.h"

#include <os_types.h>
#include <ogg.h>
#include <vorbisfile.h>
#include <vorbisenc.h>

namespace ImmCore
{
	static bool iOpenOGG(piWav *dst, piIStream *f_in)
	{
		long cumulative_read = 0;
		piTArray<uint8_t> mOutput;

		if (!mOutput.Init(2 * 44100, false))
			return false;

		ogg_int16_t convbuffer[4096]; /* take 8k out of the data segment, not the stack */
		int convsize = 4096;

		ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
		ogg_stream_state os; /* take physical pages, weld into a logical
							 stream of packets */
		ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
		ogg_packet       op; /* one raw packet of data for decode */

		vorbis_info      vi; /* struct that stores all the static vorbis bitstream
							 settings */

		vorbis_comment   vc; /* struct that stores all the bitstream user comments */
		vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
		vorbis_block     vb; /* local working space for packet->PCM decode */

        uint64_t off0 = f_in->Tell();
		char *buffer;
		int  bytes;

		/********** Decode setup ************/
		ogg_sync_init(&oy); /* Now we can read pages */

		while (1)
		{
			/* we repeat if the bitstream is chained */
			int eos = 0;
			int i;


			/* grab some data at the head of the stream. We want the first page
			(which is guaranteed to be small and only contain the Vorbis
			stream initial header) We need the first page to get the stream
			serialno. */

			/* submit a 4k block to libvorbis' Ogg layer */
			buffer = ogg_sync_buffer(&oy, 4096);
			bytes = (int)f_in->Read(buffer, 4096);
			ogg_sync_wrote(&oy, bytes);

			cumulative_read += bytes;

			/* Get the first page. */
			if (ogg_sync_pageout(&oy, &og) != 1)
			{
				/* have we simply run out of data?  If so, we're done. */
				if (bytes<4096)
					break;
                uint64_t off = f_in->Tell() - off0;
				return false;
			}

			/* Get the serial number and set up the rest of decode. */
			/* serialno first; use it to set up a logical stream */
			ogg_stream_init(&os, ogg_page_serialno(&og));

			/* extract the initial header from the first page and verify that the
			Ogg bitstream is in fact Vorbis data */

			/* I handle the initial header first instead of just having the code
			read all three Vorbis headers at once because reading the initial
			header is an easy way to identify a Vorbis bitstream and it's
			useful to see that functionality separated out. */


			vorbis_info_init(&vi);
			vorbis_comment_init(&vc);

			if (ogg_stream_pagein(&os, &og)<0)
			{
				return false;
			}


			if (ogg_stream_packetout(&os, &op) != 1)
			{
				return false;
			}

			if (vorbis_synthesis_headerin(&vi, &vc, &op)<0)
			{
				return false;
			}


			/* At this point, we're sure we're Vorbis. We've set up the logical
			(Ogg) bitstream decoder. Get the comment and codebook headers and
			set up the Vorbis decoder */


			/* The next two packets in order are the comment and codebook headers.
			They're likely large and may span multiple pages. Thus we read
			and submit data until we get our two packets, watching that no
			pages are missing. If a page is missing, error out; losing a
			header page is the only place where missing data is fatal. */

			i = 0;
			while (i<2)
			{
				while (i<2)
				{
					int result = ogg_sync_pageout(&oy, &og);
					if (result == 0)break; /* Need more data */

										   /* Don't complain about missing or corrupt data yet. We'll catch it at the packet output phase */
					if (result == 1)
					{
						ogg_stream_pagein(&os, &og); /* we can ignore any errors here
													 as they'll also become apparent
													 at packetout */
						while (i<2)
						{

							result = ogg_stream_packetout(&os, &op);
							if (result == 0)break;
							if (result<0)
							{
								return false;
							}
							result = vorbis_synthesis_headerin(&vi, &vc, &op);

							if (result<0)
							{
								return false;
							}
							i++;
						}
					}
				}

				/* no harm in not checking before adding more */
				buffer = ogg_sync_buffer(&oy, 4096);
				bytes = (int)f_in->Read((uint8_t*)buffer, 4096);

				cumulative_read += bytes;
				//Percentage done

				if (bytes == 0 && i<2)
				{
					return false;
				}
				ogg_sync_wrote(&oy, bytes);
			}


			/* Throw the comments plus a few lines about the bitstream we're decoding */
			/*{
			char **ptr=vc.user_comments;
			while(*ptr)
			{
			fprintf(stderr,"%s\n",*ptr);
			++ptr;
			}
			fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
			fprintf(stderr,"Encoded by: %s\n\n",vc.vendor);
			}*/

			convsize = 4096 / vi.channels;


			/* OK, got and parsed all three headers. Initialize the Vorbis packet->PCM decoder. */
			if (vorbis_synthesis_init(&vd, &vi) == 0)
			{ /* central decode state */
				vorbis_block_init(&vd, &vb); /* local state for most of the decode so multiple block decodes can
											 proceed in parallel. We could init
											 multiple vorbis_block structures for vd here */

											 /* The rest is just a straight decode loop until end of stream */
				while (!eos)
				{
					while (!eos)
					{
						int result = ogg_sync_pageout(&oy, &og);
						if (result == 0)
							break; /* need more data */

						if (result<0)
						{
							/* missing or corrupt data at this page position */
							// Corrupt or missing data in bitstream! Continuing...
						}
						else
						{
							ogg_stream_pagein(&os, &og); /* can safely ignore errors at this point */

							while (1)
							{
								result = ogg_stream_packetout(&os, &op);
								if (result == 0)
									break; /* need more data */

								if (result<0)
								{ /* missing or corrupt data at this page position */
								  /* no reason to complain; already complained above */
								}
								else
								{
									/* we have a packet.  Decode it */
									float **pcm;
									int samples;
									if (vorbis_synthesis(&vb, &op) == 0) /* test for success! */
										vorbis_synthesis_blockin(&vd, &vb);

									/* **pcm is a multichannel float vector.  In stereo, for
									example, pcm[0] is left, and pcm[1] is right.  samples is
									the size of each channel.  Convert the float values
									(-1.<=range<=1.) to whatever PCM format and write it out */


									while ((samples = vorbis_synthesis_pcmout(&vd, &pcm))>0)
									{
										int j;
										int clipflag = 0;
										int bout = (samples<convsize ? samples : convsize);

										/* convert floats to 16 bit signed ints (host order) and interleave */
										for (i = 0;i<vi.channels;i++)
										{
											ogg_int16_t *ptr = convbuffer + i;
											float  *mono = pcm[i];
											for (j = 0;j<bout;j++)
											{
												int val = int(mono[j] * 32767.f + .5f);

												/* might as well guard against clipping */
												if (val>32767)
												{
													val = 32767;
													clipflag = 1;
												}
												if (val<-32768)
												{
													val = -32768;
													clipflag = 1;
												}
												*ptr = val;
												ptr += vi.channels;
											}
										}

										//if(clipflag)
										//fprintf(stderr,"Clipping in frame %ld\n",(long)(vd.sequence));

										mOutput.Append((uint8_t*)convbuffer, bout * 2 * vi.channels, true);
										int written = bout;

										vorbis_synthesis_read(&vd, bout); /* tell libvorbis how many samples we actually consumed */
									}
								}
							}
							if (ogg_page_eos(&og))
								eos = 1;
						}
					}

					if (!eos)
					{
						buffer = ogg_sync_buffer(&oy, 4096);
						bytes = (int)f_in->Read((uint8_t*)buffer, 4096);

						cumulative_read += bytes;

						ogg_sync_wrote(&oy, bytes);
						if (bytes == 0)
							eos = 1;
					}
				}

				/* ogg_page and ogg_packet structs always point to storage in libvorbis.  They're never freed or manipulated directly */
				vorbis_block_clear(&vb);
				vorbis_dsp_clear(&vd);

				//Seek back and write the WAV header

				if (!dst->Make(vi.rate, vi.channels, 16, mOutput.GetAddress(0), mOutput.GetLength()))
					return false;
			}
			else
			{
				// "Error: Corrupt header during playback initialization! Aborting!"
				return false;
			}

			/* clean up this logical bitstream; before exit we see if we're followed by another [chained] */
			ogg_stream_clear(&os);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);  /* must be called last */
		}

		/* OK, clean up the framer */
		ogg_sync_clear(&oy);

		return true;
	}

	//=================================================================================

	static bool iConvert_WAVToOGG(const piWav * wav, piOStream * fp)
	{
		ogg_stream_state os;
		ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
		ogg_packet       op; /* one raw packet of data for decode */

		vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
		vorbis_comment   vc; /* struct that stores all the user comments */

		vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
		vorbis_block     vb; /* local working space for packet->PCM decode */

							 //Encode setup
		vorbis_info_init(&vi);


		if (vorbis_encode_init_vbr(&vi, wav->mNumChannels, wav->mRate, 0.9f) != 0)
			return false;

		vorbis_comment_init(&vc);
		/*
		vorbis_comment_add_tag(&vc, "ALBUM", ivc.ALBUM);
		vorbis_comment_add_tag(&vc, "ARTIST", ivc.ARTIST);
		vorbis_comment_add_tag(&vc, "CONTACT", ivc.CONTACT);
		vorbis_comment_add_tag(&vc, "COPYRIGHT", ivc.COPYRIGHT);
		vorbis_comment_add_tag(&vc, "DATE", ivc.DATE);
		vorbis_comment_add_tag(&vc, "DESCRIPTION", ivc.DESCRIPTION);
		vorbis_comment_add_tag(&vc, "GENRE", ivc.GENRE);
		vorbis_comment_add_tag(&vc, "ISRC", ivc.ISRC);
		vorbis_comment_add_tag(&vc, "LICENSE", ivc.LICENSE);
		vorbis_comment_add_tag(&vc, "LOCATION", ivc.LOCATION);
		vorbis_comment_add_tag(&vc, "ORGANISATION", ivc.ORGANISATION);
		vorbis_comment_add_tag(&vc, "PERFORMER", ivc.PERFORMER);
		vorbis_comment_add_tag(&vc, "TITLE", ivc.TITLE);
		vorbis_comment_add_tag(&vc, "TRACKNUMBER", ivc.TRACKNUMBER);
		vorbis_comment_add_tag(&vc, "VERSION", ivc.VERSION);
		*/


		//Set up the analysis state and auxiliary encoding storage
		vorbis_analysis_init(&vd, &vi);
		vorbis_block_init(&vd, &vb);

		// Set up our packet->stream encoder pick a random serial number; that way we can more likely build chained streams just by concatenation
		ogg_stream_init(&os, 0x656d6d49);

		{
			ogg_packet header;
			ogg_packet header_comm;
			ogg_packet header_code;

			vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
			ogg_stream_packetin(&os, &header); /* automatically placed in its own page */
			ogg_stream_packetin(&os, &header_comm);
			ogg_stream_packetin(&os, &header_code);

			//This ensures the actual audio data will start on a new page, as per spec
			while (true)
			{
				int result = ogg_stream_flush(&os, &og);
				if (result == 0) break;
				fp->Write(og.header, og.header_len);
				fp->Write(og.body, og.body_len);
			}
		}

		const uint64_t bytesPerSample = wav->mBits * wav->mNumChannels / 8;
		const uint64_t numWindowSamples = 256;

		uint64_t readOffset = 0;

		int eos = 0;
		while (!eos)
		{
			const int8_t *readbuffer = (int8_t*)wav->mData + readOffset*bytesPerSample;
			const int numReadSamples = static_cast<int>(((readOffset + numWindowSamples) > wav->mNumSamples) ? wav->mNumSamples - readOffset : numWindowSamples);

			readOffset += numReadSamples;

			if (numReadSamples <= 0)
			{
				vorbis_analysis_wrote(&vd, 0);
			}
			else
			{

				float** buffer = vorbis_analysis_buffer(&vd, numWindowSamples);

                if (wav->mBits == 32)
                {
                    const int bytesPerChannel = 4;
                    const int bytesPerSample = wav->mNumChannels * bytesPerChannel;
                    for (int i = 0; i < numReadSamples; i++)
                        for (int j = 0; j < wav->mNumChannels; j++)
                        buffer[j][i] = ((float*)readbuffer)[wav->mNumChannels * i + j];

                }
                // --- 24 bit formats ---
				else if (wav->mBits == 24)
				{
                    const int bytesPerChannel = 3;
                    const int bytesPerSample = wav->mNumChannels * bytesPerChannel;
					for (int i = 0; i < numReadSamples; i++)
                        for (int j = 0; j < wav->mNumChannels; j++)
						buffer[j][i] = ((readbuffer[i * bytesPerSample + j*bytesPerChannel + 2] << 16) |
                                          ((0x00ff & (int)readbuffer[i * bytesPerSample + j*bytesPerChannel + 1]) << 8) |
                                           (0x00ff & (int)readbuffer[i * bytesPerSample + j*bytesPerChannel + 0]))
                        / float(1 << 23);
				}
                // --- 16 bit formats ---
                else if (wav->mBits == 16)
                {
                    const int bytesPerChannel = 2;
                    const int bytesPerSample = wav->mNumChannels * bytesPerChannel;
                    for (int i = 0; i < numReadSamples; i++)
                       for(int j=0; j<wav->mNumChannels; j++)
                            buffer[j][i] = ((readbuffer[i * bytesPerSample + j* bytesPerChannel + 1] << 8) | (0x00ff & (int)readbuffer[i * bytesPerSample + j* bytesPerChannel + 0])) / float(1 << 15);

                }
				// --- 8 bit formats ---
				else
				{
					return false;
				}
				//tell the library how much we actually submitted
				vorbis_analysis_wrote(&vd, numReadSamples);
			}

			//vorbis does some data preanalysis, then divvies up blocks for more involved (potentially parallel) processing.
			//Get a single block for encoding now
			while (vorbis_analysis_blockout(&vd, &vb) == 1)
			{
				//analysis, assume we want to use bitrate management
				vorbis_analysis(&vb, NULL);
				vorbis_bitrate_addblock(&vb);
				while (vorbis_bitrate_flushpacket(&vd, &op))
				{
					//weld the packet into the bitstream
					ogg_stream_packetin(&os, &op);

					//write out pages (if any)
					while (!eos)
					{
						int result = ogg_stream_pageout(&os, &og);
						if (result == 0)break;
						fp->Write(og.header, og.header_len);
						fp->Write(og.body, og.body_len);
						//this could be set above, but for illustrative purposes, I do it here (to show that vorbis does know where the stream ends)
						if (ogg_page_eos(&og))eos = 1;
					}
				}
			}
		}

		ogg_stream_clear(&os);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi);

		return true;
	}


	//=================================================================================



	bool ReadOGGFromDisk(piWav *dst, const wchar_t *name)
	{
		piFile fp;
		if (!fp.Open(name, L"rb"))
			return false;

		piIStreamFile st(&fp, piIStreamFile::kDefaultFileSize);
		bool res = iOpenOGG(dst, &st);

		fp.Close();

		return res;
	}

    bool ReadOGGFromFile(piWav *dst, piFile *fp, uint64_t size)
    {
        piIStreamFile st(fp, size);
        return iOpenOGG(dst, &st);
    }

	bool ReadOGGFromMemory(piWav *dst, piTArray<uint8_t> *data)
	{
		piIStreamArray st(data);
		return iOpenOGG(dst, &st);
	}


	bool WriteOGGToMemory( piTArray<uint8_t> *data, const piWav *wav)
	{
        if (!data->Init(1024*1024, false))
            return false;

		piOStreamArray st(data);

		return iConvert_WAVToOGG(wav, &st);
	}

	bool WriteOGGToFile(const wchar_t * name, const piWav *wav)
	{
		piFile fp;
		if (!fp.Open(name, L"wb"))
			return false;

		piOStreamFile st(&fp);

		bool res = iConvert_WAVToOGG(wav, &st);

		fp.Close();

		return res;
	}
}
