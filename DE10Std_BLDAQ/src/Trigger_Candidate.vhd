library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity Trigger_Candidate is
  --Port declaration
   port (
   --Inputs
   Clock : in std_logic;
   Reset : in std_logic;

--Input stream
Data_Stream : in unsigned (31 downto 0);
--Data_Valid: in std_logic;
Data_Valid : in std_logic;

--Output
--TriggerCounterOut: out unsigned (11 downto 0)
TriggerCheck : out std_logic;
TriggerStream: out unsigned (31 downto 0)

);

end entity;

architecture algo of Trigger_Candidate is

    -- Check for the Frame with Trigger
 signal TriggerCounter : unsigned (15 downto 0)  := x"0000";
 signal WordCounter : unsigned (11 downto 0)  := x"000";    -- check the words inside a frame
 signal SensorTag : unsigned (15 downto 0)  := x"0000";
-- signal TriggerCheck : std_logic := '0';




   begin

    -- Trigger counter in DataStreamOut
    process (Clock,Reset)
    begin
      if Reset = '1' then
        WordCounter <= x"000";
        TriggerCounter<= x"0000";
      elsif rising_edge(Clock) then

        if Data_Valid = '1' then
          WordCounter <= WordCounter+1;
          if Data_Stream(31 downto 16) = x"a0a0" then -- header, 2 world of a frame, 6° if datastreamout
            WordCounter <= x"006";
            SensorTag <= Data_Stream(15 downto 0);  -- information of the sensor of the trigger
          end if;
        else
          WordCounter <= x"000";
          SensorTag <= x"0000";
        end if;

        if WordCounter >= 7 and Data_Stream (23 downto 16) = x"11" then    -- if the worlds of the signal   NB: in this case the footer is considered!
          TriggerCheck <= '1';
          TriggerCounter<= TriggerCounter+1;
          TriggerStream <= (TriggerCounter+1) & SensorTag ;     --perchè questi comandi sono fatti in parallelo, quindi in questo comando il triggercounter non è ancora stato incrementato

        else
          TriggerCheck <= '0';
          TriggerStream <= x"0000_0000";
        end if;

      end if;
    end process;



  --  TriggerCounterOut <= TriggerCounter;

  end architecture;
