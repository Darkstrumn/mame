// license:BSD-3-Clause
// copyright-holders:Curt Coder
/*


ToDo:
- peripheral interfaces

- Fix floppy. It needs to WAIT the cpu whenever port 0x14 is read, wait
  for either DRQ or INTRQ to assert, then release the cpu and then do the
  actual port read. Our Z80 cannot do that.
  The schematic isn't clear, but it seems the 2 halves of U16 (as shown) have
  a common element, so that activity on one side can affect what happens on
  the other side.
  If you uncomment the line in fdc_intrq_w, and change the BOGUSWAIT to WAIT
  in fdc_r, then load up the cpm disk (from software list), it will read the
  CP/M boot track into memory and attempt to run it. However, it has an issue
  and returns to the monitor. The other disks are useless.

*/

#include "emu.h"
#include "bus/rs232/rs232.h"
//#include "bus/s100/s100.h"
#include "includes/super6.h"
#include "softlist.h"

//**************************************************************************
//  MEMORY BANKING
//**************************************************************************

//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void super6_state::bankswitch()
{
	address_space &program = m_maincpu->space(AS_PROGRAM);
	uint8_t *ram = m_ram->pointer();

	// power on jump
	if (!BIT(m_bank0, 6)) { program.install_rom(0x0000, 0x07ff, 0xf800, m_rom); return; }

	// first 64KB of memory
	program.install_ram(0x0000, 0xffff, ram);

	// second 64KB of memory
	int map = (m_bank1 >> 4) & 0x07;

	switch (map)
	{
	case 0:
		if (BIT(m_bank1, 0)) program.install_ram(0x0000, 0x3fff, ram + 0x10000);
		if (BIT(m_bank1, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x14000);
		if (BIT(m_bank1, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x18000);
		if (BIT(m_bank1, 3)) program.install_ram(0xc000, 0xffff, ram + 0x1c000);
		break;

	case 1:
		if (BIT(m_bank1, 0)) program.install_ram(0x0000, 0x3fff, ram + 0x10000);
		if (BIT(m_bank1, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x14000);
		if (BIT(m_bank1, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x18000);
		if (BIT(m_bank1, 3)) program.install_ram(0xc000, 0xffff, ram + 0x0000);
		break;

	case 2:
		if (BIT(m_bank1, 0)) program.install_ram(0x0000, 0x3fff, ram + 0x10000);
		if (BIT(m_bank1, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x14000);
		if (BIT(m_bank1, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x4000);
		if (BIT(m_bank1, 3)) program.install_ram(0xc000, 0xffff, ram + 0x1c000);
		break;

	case 3:
		if (BIT(m_bank1, 0)) program.install_ram(0x0000, 0x3fff, ram + 0x10000);
		if (BIT(m_bank1, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x14000);
		if (BIT(m_bank1, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x0000);
		if (BIT(m_bank1, 3)) program.install_ram(0xc000, 0xffff, ram + 0x4000);
		break;

	case 4:
		if (BIT(m_bank1, 0)) program.install_ram(0x0000, 0x3fff, ram + 0xc000);
		if (BIT(m_bank1, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x14000);
		if (BIT(m_bank1, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x18000);
		if (BIT(m_bank1, 3)) program.install_ram(0xc000, 0xffff, ram + 0x1c000);
		break;
	}

	// bank 0 overrides
	if (BIT(m_bank0, 0)) program.install_ram(0x0000, 0x3fff, ram + 0x0000);
	if (BIT(m_bank0, 1)) program.install_ram(0x4000, 0x7fff, ram + 0x4000);
	if (BIT(m_bank0, 2)) program.install_ram(0x8000, 0xbfff, ram + 0x8000);
	if (BIT(m_bank0, 3)) program.install_ram(0xc000, 0xffff, ram + 0xc000);

	// PROM enabled
	if (!BIT(m_bank0, 5)) program.install_rom(0xf000, 0xf7ff, 0x800, m_rom);
}


//-------------------------------------------------
//  s100_w - S-100 bus extended address A16-A23
//-------------------------------------------------

WRITE8_MEMBER( super6_state::s100_w )
{
	/*

	    bit     description

	    0       A16
	    1       A17
	    2       A18
	    3       A19
	    4       A20
	    5       A21
	    6       A22
	    7       A23

	*/

	m_s100 = data;
}


//-------------------------------------------------
//  bank0_w - on-board memory control port #0
//-------------------------------------------------

WRITE8_MEMBER( super6_state::bank0_w )
{
	/*

	    bit     description

	    0       memory bank 0 (0000-3fff)
	    1       memory bank 1 (4000-7fff)
	    2       memory bank 2 (8000-bfff)
	    3       memory bank 3 (c000-ffff)
	    4
	    5       PROM enabled (0=enabled, 1=disabled)
	    6       power on jump reset
	    7       parity check enable

	*/

	m_bank0 = data;

	bankswitch();
}


//-------------------------------------------------
//  bank1_w - on-board memory control port #1
//-------------------------------------------------

WRITE8_MEMBER( super6_state::bank1_w )
{
	/*

	    bit     description

	    0       memory bank 4
	    1       memory bank 5
	    2       memory bank 6
	    3       memory bank 7
	    4       map select 0
	    5       map select 1
	    6       map select 2
	    7

	*/

	m_bank1 = data;

	bankswitch();
}



//**************************************************************************
//  PERIPHERALS
//**************************************************************************

//-------------------------------------------------
//  floppy_r - FDC synchronization/drive/density
//-------------------------------------------------

READ8_MEMBER( super6_state::fdc_r )
{
	/*

	    bit     description

	    0
	    1
	    2
	    3
	    4
	    5
	    6
	    7       FDC INTRQ

	*/

	m_maincpu->set_input_line(Z80_INPUT_LINE_BOGUSWAIT, ASSERT_LINE);

	return m_fdc->intrq_r() ? 0x7f : 0xff;
}


//-------------------------------------------------
//  floppy_w - FDC synchronization/drive/density
//-------------------------------------------------

WRITE8_MEMBER( super6_state::fdc_w )
{
	/*

	    bit     description

	    0       disk drive select 0
	    1       disk drive select 1
	    2       head select (0=head 1, 1=head 2)
	    3       disk density (0=single, 1=double)
	    4       size select (0=8", 1=5.25")
	    5
	    6
	    7

	    Codes passed to here during boot are 0x00, 0x08, 0x38
	*/

	// disk drive select
	floppy_image_device *floppy = nullptr;

	if ((data & 3) == 0)
		floppy = m_floppy0->get_device();
	if ((data & 3) == 1)
		floppy = m_floppy1->get_device();

	m_fdc->set_floppy(floppy);
	if (floppy) floppy->mon_w(0);

	// head select
	if (floppy) floppy->ss_w(BIT(data, 2));

	// disk density
	m_fdc->dden_w(!BIT(data, 3));

	// disk size
	m_fdc->set_unscaled_clock (BIT(data, 4) ? 1'000'000 : 2'000'000);  // division occurs inside fdc depending on ENMF
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( super6_mem )
//-------------------------------------------------

void super6_state::super6_mem(address_map &map)
{
}


//-------------------------------------------------
//  ADDRESS_MAP( super6_io )
//-------------------------------------------------

void super6_state::super6_io(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();
	map(0x00, 0x03).rw(m_dart, FUNC(z80dart_device::ba_cd_r), FUNC(z80dart_device::ba_cd_w));
	map(0x04, 0x07).rw(m_pio, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
	map(0x08, 0x0b).rw(m_ctc, FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
	map(0x0c, 0x0f).rw(m_fdc, FUNC(wd2793_device::read), FUNC(wd2793_device::write));
	map(0x10, 0x10).mirror(0x03).rw(m_dma, FUNC(z80dma_device::bus_r), FUNC(z80dma_device::bus_w));
	map(0x14, 0x14).rw(FUNC(super6_state::fdc_r), FUNC(super6_state::fdc_w));
	map(0x15, 0x15).portr("J7").w(FUNC(super6_state::s100_w));
	map(0x16, 0x16).w(FUNC(super6_state::bank0_w));
	map(0x17, 0x17).w(FUNC(super6_state::bank1_w));
	map(0x18, 0x18).mirror(0x03).w(BR1945_TAG, FUNC(com8116_device::stt_str_w));
//  AM_RANGE(0x40, 0x40) ?
//  AM_RANGE(0xe0, 0xe7) HDC?
}



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( super6 )
//-------------------------------------------------

static INPUT_PORTS_START( super6 )
	PORT_START("J7")
	PORT_DIPNAME( 0x0f, 0x0e, "SIO Channel A Baud Rate" ) PORT_DIPLOCATION("J7:1,2,3,4")
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x01, "75" )
	PORT_DIPSETTING(    0x02, "110" )
	PORT_DIPSETTING(    0x03, "134.5" )
	PORT_DIPSETTING(    0x04, "150" )
	PORT_DIPSETTING(    0x05, "300" )
	PORT_DIPSETTING(    0x06, "600" )
	PORT_DIPSETTING(    0x07, "1200" )
	PORT_DIPSETTING(    0x08, "1800" )
	PORT_DIPSETTING(    0x09, "2000" )
	PORT_DIPSETTING(    0x0a, "2400" )
	PORT_DIPSETTING(    0x0b, "3600" )
	PORT_DIPSETTING(    0x0c, "4800" )
	PORT_DIPSETTING(    0x0d, "7200" )
	PORT_DIPSETTING(    0x0e, "9600" )
	PORT_DIPSETTING(    0x0f, "19200" )
	PORT_DIPNAME( 0x70, 0x70, "SIO Channel B Baud Rate" ) PORT_DIPLOCATION("J7:5,6,7")
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x10, "75" )
	PORT_DIPSETTING(    0x20, "110" )
	PORT_DIPSETTING(    0x30, "134.5" )
	PORT_DIPSETTING(    0x40, "150" )
	PORT_DIPSETTING(    0x50, "300" )
	PORT_DIPSETTING(    0x60, "600" )
	PORT_DIPSETTING(    0x70, "1200" )
	PORT_DIPNAME( 0x80, 0x00, "Disk Drive Type" ) PORT_DIPLOCATION("J7:8")
	PORT_DIPSETTING(    0x80, "Single Sided" )
	PORT_DIPSETTING(    0x00, "Double Sided" )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80CTC
//-------------------------------------------------


//-------------------------------------------------
//  Z80DMA
//-------------------------------------------------

READ8_MEMBER(super6_state::memory_read_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	return prog_space.read_byte(offset);
}

WRITE8_MEMBER(super6_state::memory_write_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM);
	prog_space.write_byte(offset, data);
}

READ8_MEMBER(super6_state::io_read_byte)
{
	address_space& prog_space = m_maincpu->space(AS_IO);
	return prog_space.read_byte(offset);
}

WRITE8_MEMBER(super6_state::io_write_byte)
{
	address_space& prog_space = m_maincpu->space(AS_IO);
	prog_space.write_byte(offset, data);
}


//-------------------------------------------------
//  floppy_format_type floppy_formats
//-------------------------------------------------

static void super6_floppies(device_slot_interface &device)
{
	device.option_add("525dd", FLOPPY_525_QD);
}

WRITE_LINE_MEMBER( super6_state::fdc_intrq_w )
{
	if (state) m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, CLEAR_LINE);

	m_ctc->trg3(state);   // J6 pin 7-8
	// m_maincpu->set_state_int(Z80_AF, 0x7f00);   // hack, see notes
}

WRITE_LINE_MEMBER( super6_state::fdc_drq_w )
{
	if (state) m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, CLEAR_LINE);

	m_dma->rdy_w(state);
}


//-------------------------------------------------
//  z80_daisy_config super6_daisy_chain
//-------------------------------------------------

// no evidence of daisy chain in use - removed for now
//static const z80_daisy_config super6_daisy_chain[] =
//{
//	{ Z80CTC_TAG },
//	{ Z80DART_TAG },
//	{ Z80PIO_TAG },
//	{ nullptr }
//};


//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( super6 )
//-------------------------------------------------

void super6_state::machine_start()
{
	// state saving
	save_item(NAME(m_s100));
	save_item(NAME(m_bank0));
	save_item(NAME(m_bank1));
}


void super6_state::machine_reset()
{
	m_bank0 = m_bank1 = 0;

	bankswitch();
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( super6 )
//-------------------------------------------------

MACHINE_CONFIG_START(super6_state::super6)
	// basic machine hardware
	MCFG_DEVICE_ADD(m_maincpu, Z80, 24_MHz_XTAL / 4)
	MCFG_DEVICE_PROGRAM_MAP(super6_mem)
	MCFG_DEVICE_IO_MAP(super6_io)
	//MCFG_Z80_DAISY_CHAIN(super6_daisy_chain)

	// devices
	MCFG_DEVICE_ADD(m_ctc, Z80CTC, 24_MHz_XTAL / 4)
	MCFG_Z80CTC_ZC0_CB(WRITELINE(m_ctc, z80ctc_device, trg1))     // J6 pin 2-3
	MCFG_Z80CTC_INTR_CB(INPUTLINE(Z80_TAG, INPUT_LINE_IRQ0))

	clock_device &ctc_tick(CLOCK(config, "ctc_tick", 24_MHz_XTAL / 16));
	ctc_tick.signal_handler().set(m_ctc, FUNC(z80ctc_device::trg0));   // J6 pin 1-14 (1.5MHz)

	MCFG_DEVICE_ADD(m_dma, Z80DMA, 24_MHz_XTAL / 6)
	MCFG_Z80DMA_OUT_BUSREQ_CB(WRITELINE(m_dma, z80dma_device, bai_w))
	MCFG_Z80DMA_OUT_INT_CB(WRITELINE(m_ctc, z80ctc_device, trg2))
	MCFG_Z80DMA_IN_MREQ_CB(READ8(*this, super6_state, memory_read_byte))
	MCFG_Z80DMA_OUT_MREQ_CB(WRITE8(*this, super6_state, memory_write_byte))
	MCFG_Z80DMA_IN_IORQ_CB(READ8(*this, super6_state, io_read_byte))
	MCFG_Z80DMA_OUT_IORQ_CB(WRITE8(*this, super6_state, io_write_byte))

	MCFG_DEVICE_ADD(m_pio, Z80PIO, 24_MHz_XTAL / 4)
	MCFG_Z80PIO_OUT_INT_CB(INPUTLINE(Z80_TAG, INPUT_LINE_IRQ0))

	MCFG_DEVICE_ADD(m_fdc, WD2793, 24_MHz_XTAL / 12)
	MCFG_WD_FDC_FORCE_READY
	MCFG_WD_FDC_INTRQ_CALLBACK(WRITELINE(*this, super6_state, fdc_intrq_w))
	MCFG_WD_FDC_DRQ_CALLBACK(WRITELINE(*this, super6_state, fdc_drq_w))

	MCFG_FLOPPY_DRIVE_ADD(m_floppy0, super6_floppies, "525dd", floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)
	MCFG_FLOPPY_DRIVE_ADD(m_floppy1, super6_floppies, nullptr, floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_SOUND(true)

	MCFG_DEVICE_ADD(m_dart, Z80DART, 24_MHz_XTAL / 4)
	MCFG_Z80DART_OUT_TXDA_CB(WRITELINE(RS232_A_TAG, rs232_port_device, write_txd))
	MCFG_Z80DART_OUT_DTRA_CB(WRITELINE(RS232_A_TAG, rs232_port_device, write_dtr))
	MCFG_Z80DART_OUT_RTSA_CB(WRITELINE(RS232_A_TAG, rs232_port_device, write_rts))
	MCFG_Z80DART_OUT_TXDB_CB(WRITELINE(RS232_B_TAG, rs232_port_device, write_txd))
	MCFG_Z80DART_OUT_DTRB_CB(WRITELINE(RS232_B_TAG, rs232_port_device, write_dtr))
	MCFG_Z80DART_OUT_RTSB_CB(WRITELINE(RS232_B_TAG, rs232_port_device, write_rts))
	MCFG_Z80DART_OUT_INT_CB(INPUTLINE(Z80_TAG, INPUT_LINE_IRQ0))

	MCFG_DEVICE_ADD(RS232_A_TAG, RS232_PORT, default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(WRITELINE(m_dart, z80dart_device, rxa_w))

	MCFG_DEVICE_ADD(RS232_B_TAG, RS232_PORT, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(WRITELINE(m_dart, z80dart_device, rxb_w))

	COM8116(config, m_brg, 5.0688_MHz_XTAL);
	m_brg->fr_handler().set(m_dart, FUNC(z80dart_device::txca_w));
	m_brg->fr_handler().append(m_dart, FUNC(z80dart_device::rxca_w));
	m_brg->fr_handler().append(m_ctc, FUNC(z80ctc_device::trg1));
	m_brg->ft_handler().set(m_dart, FUNC(z80dart_device::rxtxcb_w));

	// internal ram
	RAM(config, RAM_TAG).set_default_size("128K");

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "super6")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( super6 )
//-------------------------------------------------

ROM_START( super6 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "v36" )
	ROM_SYSTEM_BIOS( 0, "v36", "ADC S6 v3.6" )
	ROMX_LOAD( "adcs6_v3.6.u29", 0x000, 0x800, CRC(386fd22a) SHA1(9c177990aa180ab93be9c4641e92ae934627e661), ROM_BIOS(0) )
	ROM_SYSTEM_BIOS( 1, "v12", "Digitex Monitor v1.2a" )
	ROMX_LOAD( "digitex monitor 1.2a 6oct1983.u29", 0x000, 0x800, CRC(a4c33ce4) SHA1(46dde43ea51d295f2b3202c2d0e1883bde1a8da7), ROM_BIOS(1) )

	ROM_REGION( 0x800, "plds", 0 )
	ROM_LOAD( "pal16l8.u16", 0x000, 0x800, NO_DUMP )
	ROM_LOAD( "pal16l8.u36", 0x000, 0x800, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT        COMPANY                         FULLNAME     FLAGS
COMP( 1983, super6, 0,      0,      super6,  super6, super6_state, empty_init, "Advanced Digital Corporation", "Super Six", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
