// license: BSD-3-Clause
// copyright-holders: Dirk Best
/***************************************************************************

	Juku E5101

	Hardware:
	- КР580ВМ80A
	- КР580ИР82
	- КР580ВА86
	- КР580ВА87
	- КР580ВИ53 x3
	- КР580ВК38
	- КР580ВН59
	- КР580ВВ51A x2
	- КР580ВВ55A x2

***************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/bankdev.h"
#include "machine/i8251.h"
#include "machine/i8255.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "screen.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class juku_state : public driver_device
{
public:
	juku_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_bank(*this, "bank"),
		m_pic(*this, "pic"),
		m_pit(*this, "pit%u", 0U),
		m_pio(*this, "pio%u", 0U),
		m_sio(*this, "sio%u", 0U)
	{ }

	void juku(machine_config &config);

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	required_device<cpu_device> m_maincpu;
	required_device<address_map_bank_device> m_bank;
	required_device<pic8259_device> m_pic;
	required_device_array<pit8253_device, 3> m_pit;
	required_device_array<i8255_device, 2> m_pio;
	required_device_array<i8251_device, 2> m_sio;

	void mem_map(address_map &map);
	void bank_map(address_map &map);
	void io_map(address_map &map);

	void pio0_portc_w(uint8_t data);

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	std::unique_ptr<uint8_t[]> m_ram;
};


//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

void juku_state::mem_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_bank, FUNC(address_map_bank_device::amap8));
}

void juku_state::bank_map(address_map &map)
{
	// memory mode 0
	map(0x00000, 0x03fff).rom().region("maincpu", 0);
	map(0x04000, 0x0ffff).bankrw("ram_4000");
	// memory mode 1
	map(0x10000, 0x1ffff).bankrw("ram_0000");
	map(0x1d800, 0x1ffff).bankr("rom_d800");
	// memory mode 2
	map(0x20000, 0x23fff).bankrw("ram_0000");
	map(0x24000, 0x2bfff).noprw(); // extension
	map(0x2c000, 0x2ffff).bankrw("ram_c000");
	map(0x2d800, 0x2ffff).bankr("rom_d800");
	// memory mode 3
	map(0x30000, 0x3ffff).bankrw("ram_0000");
}

void juku_state::io_map(address_map &map)
{
	map(0x00, 0x01).rw(m_pic, FUNC(pic8259_device::read), FUNC(pic8259_device::write));
	map(0x04, 0x07).rw(m_pio[0], FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x08, 0x0b).rw(m_sio[0], FUNC(i8251_device::read), FUNC(i8251_device::write));
	map(0x0c, 0x0f).rw(m_pio[1], FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x10, 0x13).rw(m_pit[0], FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0x14, 0x17).rw(m_pit[1], FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0x18, 0x1b).rw(m_pit[2], FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0x1c, 0x1f).rw(m_sio[1], FUNC(i8251_device::read), FUNC(i8251_device::write));
}


//**************************************************************************
//  INPUT PORT DEFINITIONS
//**************************************************************************

static INPUT_PORTS_START( juku )
INPUT_PORTS_END


//**************************************************************************
//  VIDEO
//**************************************************************************

uint32_t juku_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	for (int y = 0; y < 240; y++)
		for (int x = 0; x < 320; x++)
			bitmap.pix32(y, x) = BIT(m_ram[0xd800 + (y * (320 / 8) + x / 8)], 7 - (x % 8)) ? rgb_t::white() : rgb_t::black();

	return 0;
}


//**************************************************************************
//  MACHINE EMULATION
//**************************************************************************

void juku_state::pio0_portc_w(uint8_t data)
{
	m_bank->set_bank(data & 0x03);
}

void juku_state::machine_start()
{
	m_ram = std::make_unique<uint8_t[]>(0x10000);

	membank("rom_d800")->set_base(memregion("maincpu")->base() + 0x1800);

	membank("ram_0000")->set_base(&m_ram[0x0000]);
	membank("ram_4000")->set_base(&m_ram[0x4000]);
	membank("ram_c000")->set_base(&m_ram[0xc000]);
}

void juku_state::machine_reset()
{
	m_bank->set_bank(0);
}


//**************************************************************************
//  MACHINE DEFINTIONS
//**************************************************************************

void juku_state::juku(machine_config &config)
{
	// КР580ВМ80A @ 2 MHz
	I8080A(config, m_maincpu, 2000000);
	m_maincpu->set_addrmap(AS_PROGRAM, &juku_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &juku_state::io_map);
	m_maincpu->set_irq_acknowledge_callback("pic", FUNC(pic8259_device::inta_cb));

	ADDRESS_MAP_BANK(config, m_bank);
	m_bank->set_map(&juku_state::bank_map);
	m_bank->set_data_width(8);
	m_bank->set_addr_width(18);
	m_bank->set_stride(0x10000);

	// КР580ВН59
	PIC8259(config, m_pic, 0);
	m_pic->out_int_callback().set_inputline(m_maincpu, 0);

	// КР580ВИ53
	PIT8253(config, m_pit[0], 0);
	m_pit[0]->set_clk<0>(16_MHz_XTAL/16);
	m_pit[0]->out_handler<0>().set(m_pit[1], FUNC(pit8253_device::write_clk0));
	m_pit[0]->set_clk<1>(16_MHz_XTAL/16);
	m_pit[0]->set_clk<2>(16_MHz_XTAL/16);
	m_pit[0]->out_handler<2>().set(m_pit[1], FUNC(pit8253_device::write_clk1));
	m_pit[0]->out_handler<2>().append(m_pit[1], FUNC(pit8253_device::write_clk2));

	// КР580ВИ53
	PIT8253(config, m_pit[1], 0);
	m_pit[1]->out_handler<1>().set(m_pic, FUNC(pic8259_device::ir5_w));

	// КР580ВИ53
	PIT8253(config, m_pit[2], 0);

	// КР580ВВ55A
	I8255A(config, m_pio[0]);
	m_pio[0]->out_pc_callback().set(FUNC(juku_state::pio0_portc_w));

	// КР580ВВ55A
	I8255A(config, m_pio[1]);

	// КР580ВВ51A
	I8251(config, m_sio[0], 0);
	m_sio[0]->rxrdy_handler().set("pic", FUNC(pic8259_device::ir2_w));
	m_sio[0]->txrdy_handler().set("pic", FUNC(pic8259_device::ir3_w));

	// КР580ВВ51A
	I8251(config, m_sio[1], 0);
	m_sio[1]->rxrdy_handler().set("pic", FUNC(pic8259_device::ir0_w));
	m_sio[1]->txrdy_handler().set("pic", FUNC(pic8259_device::ir1_w));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500)); // not accurate
	screen.set_size(320, 240);
	screen.set_visarea(0, 319, 0, 239);
	screen.set_screen_update(FUNC(juku_state::screen_update));
}


//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( juku )
	ROM_REGION(0x4000, "maincpu", 0)
	ROM_LOAD("jukurom0.bin", 0x0000, 0x2000, CRC(b26f5080) SHA1(db8bab6ff7143be890d6aaa25d10386dfdac3fc7))
	ROM_LOAD("jukurom1.bin", 0x2000, 0x2000, CRC(b184e253) SHA1(d169acde61f643d7d0780cca0eeaf33ebdf75b92))

	ROM_REGION(0x2000, "basic", 0)
	ROM_LOAD("bas0.bin", 0x0000, 0x0800, CRC(c03996cd) SHA1(3c45537c2a1879998e5315b79eb44dcf7c007d69))
	ROM_LOAD("bas1.bin", 0x0800, 0x0800, CRC(d8016869) SHA1(baef9e9c55171a9192bc13d48e3b45394c7780d9))
	ROM_LOAD("bas2.bin", 0x1000, 0x0800, CRC(9a958621) SHA1(08baca27e1ccdb0a441706df267c1f82b82d56ab))
	ROM_LOAD("bas3.bin", 0x1800, 0x0800, CRC(d4ffbf67) SHA1(bced7ff2420f630dbd4cd1c0c83481ed874869f1))
ROM_END


//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT  CLASS       INIT        COMPANY   FULLNAME      FLAGS
COMP( 1988, juku, 0,      0,      juku,    juku,  juku_state, empty_init, "Estron", "Juku E5101", MACHINE_NOT_WORKING | MACHINE_NO_SOUND )