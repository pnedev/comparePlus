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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;

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
--  | Data ...                                                      |
--  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;

library work;

-------------------------------------------------------------------------------
-- IP_snd entity
-------------------------------------------------------------------------------
entity ip_snd is
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        header_in   : in  ip_t;        
        data_in     : in  std_logic_vector(7 downto 0);
        sof_in      : in  std_logic;
        eof_in      : in  std_logic;
        src_rdy_in  : in  std_logic;
        dst_rdy_out : out std_logic;
        data_out    : out std_logic_vector(7 downto 0);
        sof_out     : out std_logic;
        eof_out     : out std_logic;
        src_rdy_out : out std_logic;
        dst_rdy_in  : in  std_logic
    );
end entity;
