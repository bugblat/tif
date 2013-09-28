-----------------------------------------------------------------------
-- flashctl.vhd
--
-- Initial entry: 21-Apr-11 te
--
-- VHDL hierarchy is
--      flasher         top level
--      tiffla.vhd        does the work!
--      tifwb.vhd         wishbone interface
--        efb.vhd           XO2 embedded function block
--
-----------------------------------------------------------------------
--
-- Copyright (c) 2001 to 2013  te
--
-----------------------------------------------------------------------
library IEEE;       use IEEE.std_logic_1164.all;
library work;       use work.defs.all;
library machxo2;    use machxo2.components.all;

--=====================================================================
entity flasher is
   port ( i2c_SCL,
          i2c_SDA       : inout std_logic;
          GSRn,
          SUSPEND       : in    std_logic;
          RedLedPad,
          GreenLedPad   : out   std_logic   );
end flasher;

--=====================================================================
architecture rtl of flasher is
  -----------------------------------------------
  component tif_flasher is port (
      red,
      green,
      xclk          : out   std_logic       );
  end component tif_flasher;
  -----------------------------------------------
  component tifwb is port (
      i2c_SCL       : inout std_logic;
      i2c_SDA       : inout std_logic;
      xclk          : in    std_logic;
      XI            : out   XIrec;
      XO            : in    slv8            );
  end component tifwb;
  -----------------------------------------------
  component tifctl is port (
      xclk          : in    std_logic;
      XI            : in    XIrec;
      XO            : out   slv8;
      MiscReg       : out   TMisc           );
  end component tifctl;
  -----------------------------------------------

  signal  red_flash,
          green_flash,
          xclk        : std_logic     := '0';
  signal  XI          : XIrec         := (  PRdFinished => false
                                         ,  PWr         => false
                                         ,  PRWA        => 0
                                         ,  PRdSubA     => 0
                                         ,  PD          => (others=>'0'));
  signal  XO          : slv8          := (others=>'0');

  signal  GSRnX,
          SuspendX    : std_logic;
  signal  MiscReg     : TMisc;

  -- attach a pullup to the GSRn signal and pulldown to SUSPEND
  attribute pullmode  : string;
  attribute pullmode of GSRnX   : signal is "UP";   -- else floats
  attribute pullmode of SuspendX: signal is "DOWN"; -- else floats

begin
  -- global reset
  IBgsr   : IB  port map ( I=>GSRn, O=>GSRnX );
  GSR_GSR : GSR port map ( GSR=>GSRnX );
  -- USB suspend signal
  IBsusp  : IB  port map ( I=>SUSPEND, O=>SuspendX );

  -----------------------------------------------
  -- wishbone interface
  WB: tifwb      port map ( i2c_SCL     => i2c_SCL,
                            i2c_SDA     => i2c_SDA,
                            xclk        => xclk,
                            XI          => XI,
                            XO          => XO           );

  -----------------------------------------------
  -- LED flasher
  F: tif_flasher port map ( red         => red_flash,
                            green       => green_flash,
                            xclk        => xclk         );

  -----------------------------------------------
  -- control logic
  TC: tifctl     port map ( xclk        => xclk,
                            XI          => XI,
                            XO          => XO,
                            MiscReg     => MiscReg      );

  -----------------------------------------------
  -- drive the LEDs
  LED_BLOCK : block
    signal r,g : std_logic;
  begin
    r <= '0'          when SuspendX='1'            else
         red_flash    when MiscReg=LED_ALTERNATING else
         red_flash    when MiscReg=LED_SYNC        else
         '0';
    g <= '0'          when SuspendX='1'            else
         green_flash  when MiscReg=LED_ALTERNATING else
         red_flash    when MiscReg=LED_SYNC        else
         '0';
    RED_BUF: OB port map ( I=>r, O => RedLedPad   );
    GRN_BUF: OB port map ( I=>g, O => GreenLedPad );
  end block LED_BLOCK;

end rtl;
-- EOF ----------------------------------------------------------------
