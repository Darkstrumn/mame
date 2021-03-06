// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, hap, superctr, cam900
/***************************************************************************

    Taito Zoom ZSG-2 sound board

***************************************************************************/
#ifndef MAME_AUDIO_TAITO_ZM_H
#define MAME_AUDIO_TAITO_ZM_H

#pragma once

#include "cpu/mn10200/mn10200.h"
#include "cpu/tms57002/tms57002.h"
#include "sound/zsg2.h"

#define USE_DSP               // comment out this to disable DSP emulation

class taito_zoom_device : public device_t, public device_mixer_interface

{
public:
	static constexpr feature_type imperfect_features() { return feature::SOUND; }

	taito_zoom_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	DECLARE_WRITE16_MEMBER(sound_irq_w);
	DECLARE_READ16_MEMBER(sound_irq_r);
	DECLARE_WRITE16_MEMBER(reg_data_w);
	DECLARE_WRITE16_MEMBER(reg_address_w);

	DECLARE_READ8_MEMBER(shared_ram_r);
	DECLARE_WRITE8_MEMBER(shared_ram_w);
	DECLARE_READ8_MEMBER(tms_ctrl_r);
	DECLARE_WRITE8_MEMBER(tms_ctrl_w);

	void set_use_flash() { m_use_flash = true; }

	void taitozoom_mn_map(address_map &map);
#ifdef USE_DSP
	void tms57002_map(address_map &map);
#endif
protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;

private:
	// inherited devices/pointers
	required_device<mn10200_device> m_soundcpu;
	required_device<tms57002_device> m_tms57002;
	required_device<zsg2_device> m_zsg2;

	// internal state
	uint16_t m_reg_address;
	uint8_t m_tms_ctrl;
	bool m_use_flash;
	std::unique_ptr<uint8_t[]> m_snd_shared_ram;
	
	void update_status_pin(int state);
};

DECLARE_DEVICE_TYPE(TAITO_ZOOM, taito_zoom_device)

#define MCFG_TAITO_ZOOM_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, TAITO_ZOOM, 0)

#define MCFG_TAITO_ZOOM_USE_FLASH \
	downcast<taito_zoom_device *>(device)->set_use_flash();

#endif // MAME_AUDIO_TAITO_ZM_H
