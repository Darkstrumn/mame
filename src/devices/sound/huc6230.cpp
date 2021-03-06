// license:BSD-3-Clause
// copyright-holders:cam900
/*
    Hudson HuC6230 SoundBox
	HuC6280 PSG with ADPCM

	TODO:
	- Volume is linear?
	- Make it actually working
	- Implement CDDA Volume
*/

#include "emu.h"
#include "huc6230.h"

static const int clamp(int val, int min, int max) { return std::min(max,std::max(min,val)); }


void huc6230_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	/* Clear buffer */

	int frq = (1 << m_adpcm_freq);
	for (int i = 0; i < samples; i++)
	{
		outputs[0][i] = inputs[0][i];
		outputs[1][i] = inputs[1][i];

		int cdda_l = inputs[2][i];
		int cdda_r = inputs[3][i];
		outputs[0][i] = clamp(outputs[0][i] + ((cdda_l * m_cdda_lvol) >> 6), -32768, 32767);
		outputs[1][i] = clamp(outputs[1][i] + ((cdda_r * m_cdda_rvol) >> 6), -32768, 32767);

		for (int adpcm = 0; adpcm < 2; adpcm++)
		{
			adpcm_channel *channel = &m_adpcm_channel[adpcm];

			if (!channel->m_playing)
				continue;

			int32_t sample;

			channel->m_pos++;
			channel->m_input = m_adpcm_update_cb[adpcm]();

			if (channel->m_pos > frq)
			{
				channel->m_pos = 0;
				channel->m_prev_sample = channel->m_curr_sample;
				channel->m_curr_sample = channel->m_adpcm.clock(channel->m_input & 0xf);
			}
			if (!channel->m_interpolate)
				sample = channel->m_curr_sample;
			else
				sample = ((channel->m_prev_sample * (frq - channel->m_pos)) + 
					(channel->m_curr_sample * channel->m_pos)) >> m_adpcm_freq;

			outputs[0][i] = clamp(outputs[0][i] + ((sample * channel->m_lvol) >> 2), -32768, 32767);
			outputs[1][i] = clamp(outputs[1][i] + ((sample * channel->m_rvol) >> 2), -32768, 32767);
		}
	}
}


/*--------------------------------------------------------------------------*/
/* MAME specific code                                                       */
/*--------------------------------------------------------------------------*/

WRITE8_MEMBER( huc6230_device::write )
{
	/* Update stream */
	m_stream->update();
	if (offset < 0x10)
	{
		m_psg->c6280_w(space, offset, data, mem_mask);
	}
	else if (offset < 0x11)
	{
		m_adpcm_freq = data & 3;
		for (int i = 0; i < 2; i++)
		{
			m_adpcm_channel[i].m_interpolate = BIT(data, 2+i);
			if (BIT(data, 4+i))
			{
				m_adpcm_channel[i].m_adpcm.reset();
				m_adpcm_channel[i].m_playing = 0;
				m_adpcm_channel[i].m_prev_sample = 0;
				m_adpcm_channel[i].m_curr_sample = 0;
				m_adpcm_channel[i].m_pos = 0;
			}
			else
			{
				m_adpcm_channel[i].m_playing = 1;
			}
		}
	}
	else if (offset < 0x15)
	{
		offset -= 0x11;
		if ((offset & 1) == 0)
			m_adpcm_channel[offset >> 1].m_lvol = data & 0x3f;
		else
			m_adpcm_channel[offset >> 1].m_rvol = data & 0x3f;
	}
	else if (offset < 0x16)
		m_cdda_lvol = data & 0x3f;
	else if (offset < 0x17)
		m_cdda_rvol = data & 0x3f;
}

DEFINE_DEVICE_TYPE(HuC6230, huc6230_device, "huc6230", "Hudson Soft HuC6230 SoundBox")

huc6230_device::huc6230_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, HuC6230, tag, owner, clock)
	, device_sound_interface(mconfig, *this)
	, m_stream(nullptr)
	, m_psg(*this, "psg")
	, m_adpcm_freq(0)
	, m_cdda_lvol(0)
	, m_cdda_rvol(0)
	, m_adpcm_update_cb{{*this}, {*this}}
{
}

//-------------------------------------------------
//  device_add_mconfig - add machine configuration
//-------------------------------------------------
void huc6230_device::device_add_mconfig(machine_config &config)
{
	C6280(config, m_psg, DERIVED_CLOCK(1,6));
	m_psg->add_route(0, DEVICE_SELF, 1.0, 0);
	m_psg->add_route(1, DEVICE_SELF, 1.0, 1);
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void huc6230_device::device_start()
{
	for (auto &cb : m_adpcm_update_cb)
		cb.resolve_safe(0);

	m_stream = machine().sound().stream_alloc(*this, 4, 2, clock() / 682); // or /672?

	for (int i = 0; i < 2; i++)
	{
		m_adpcm_channel[i].m_playing = 0;
		m_adpcm_channel[i].m_prev_sample = 0;
		m_adpcm_channel[i].m_curr_sample = 0;
		m_adpcm_channel[i].m_pos = 0;
		m_adpcm_channel[i].m_input = 0;

		save_item(NAME(m_adpcm_channel[i].m_adpcm.m_signal), i);
		save_item(NAME(m_adpcm_channel[i].m_adpcm.m_step), i);
		save_item(NAME(m_adpcm_channel[i].m_lvol), i);
		save_item(NAME(m_adpcm_channel[i].m_rvol), i);
		save_item(NAME(m_adpcm_channel[i].m_interpolate), i);
		save_item(NAME(m_adpcm_channel[i].m_playing), i);
		save_item(NAME(m_adpcm_channel[i].m_prev_sample), i);
		save_item(NAME(m_adpcm_channel[i].m_curr_sample), i);
		save_item(NAME(m_adpcm_channel[i].m_pos), i);
		save_item(NAME(m_adpcm_channel[i].m_input), i);
	}
	save_item(NAME(m_adpcm_freq));
	save_item(NAME(m_cdda_lvol));
	save_item(NAME(m_cdda_rvol));
}

void huc6230_device::device_clock_changed()
{
	m_stream->set_sample_rate(clock() / 682);
}
