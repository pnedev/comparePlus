-------------------------------------------------------------------------------
--
--  Note: IP packet
--
--   0                   1                   2                   3
--   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  |Version|  LEN  |Type of service|              Length           |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  |           Identifier          |Flags|     Fragment Offset     |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  |      TTL      |   Protocol    |           Checksum            |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  |                            Source                             |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  |                          Destination                          |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--  | Option ...  | Padding ...                                     |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

library work;
use work.ethernet_package.all;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;
        data_in     : in  std_logic_vector(15 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(15 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

-------------------------------------------------------------------------------
-- RTL architecture
-------------------------------------------------------------------------------
architecture rtl of ip_snd is

    type state_t is (wait_sof, pass_header, pass_data);
    signal state : state_t := wait_sof;
    signal ip : ip_t;
    signal cnt : std_logic_vector(7 downto 0);
    signal checksum_long : std_logic_vector(16 downto 0);
    signal checksum_i    : std_logic_vector(15 downto 0);
    signal checksum      : std_logic_vector(15 downto 0);

begin

    ----------------------------------------------------------------------------
    -- frame byte counter
    ----------------------------------------------------------------------------
    counter_p : process(rst, clk)
    begin
        if (rst = '1') then
            cnt <= (others => '0');
        elsif rising_edge(clk) then
            if (state = pass_header and dst_rdy_in = '0') then
                cnt <= cnt + 1;
            else
                cnt <= (others => '0');
            end if;
        end if;
    end process;

    checksum_i <= checksum_long(15 downto 0) + checksum_long(16);
    checksum <= not(checksum_i);

end architecture rtl;
