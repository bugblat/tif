-----------------------------------------------------------------------
-- tifcfg.vhd, 1200 version
--
-- Initial entry: 01-Ju1-13 te
-- non-common definitions to personalise the tif implementations
--
-----------------------------------------------------------------------
library ieee;                   use ieee.std_logic_1164.all;

package tifcfg is

  -- tif1200/4000 = 41h/42h = A/B
  constant TIF_ID      : std_logic_vector(7 downto 0) := x"41"; -- 'A'
  constant XO2_DENSITY : string                       := "1200L";

end package tifcfg;

-----------------------------------------------------------------------
package body tifcfg is
end package body tifcfg;

-----------------------------------------------------------------------
-- EOF tifcfg.vhd
