library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--Entity Random_HeaderFooter generates a stream of 32bit words
--When the trigger signal goes from 1 to 0 the machine goes to running mode
--First a constant sequence (header) is generated
--Then a 32bit packet containing the length in words of the total sequence considering also header and footer
--Now the output is a random 32bit number (random stream)
--The sequence concludes with a constant ending part (footer)

entity FrameSimulator is
  generic (
    headerWord : unsigned (31 downto 0) := x"a0a0_0000";
    footerWord : unsigned (31 downto 0) := x"e0e0_0000";
    sensorTag  : unsigned (15 downto 0) := x"0101";
    periodicity : natural := 200
    );
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
    Reset : in std_logic;
    Trigger : in std_logic;   --from ext
    --Outputs
    Data_Stream : out unsigned (31 downto 0);
    Data_Valid : out std_logic;
    --Is '1' if that is the last word of the sequence:
    EndOfEvent : out std_logic
    );
end entity;



architecture algorithm of FrameSimulator is

  --Two Random_Generators of 16bit width are used to create the randomic part of the process--
  component Random_generator
    port (
      --Inputs
      Clock : in std_logic;
      Reset : in std_logic;
      Enable : in std_logic;
      --Outputs
      Random_Data : out unsigned (15 downto 0)
      );
  end component;

  --Internal Counter declaration
  component Counter_nbit
    generic ( nbit : natural := 8);
    port(
      Clock: in std_logic;
      Reset: in std_logic;
      Count : out unsigned (nbit-1 downto 0)
      );
  end component;









  --Possible states of the FSM
  --Idle: machine is waiting for trigger to go from 0 to 1 to start
  --LengthDeclaration : 32bit word containing the length of the total stream is the output
  --Header: Data_Stream sends the header's words sequentially
  --Frame: Data_Stream sends the n° of the frame from the first which was sent
  --Sequence : output is now a random sequence of 32bit words
  --Footer : the sequence ends with a constant ending sequence (footer)
  type state is (idle,lengthDeclaration,header,frame,sequence,footer);

  ----  Internal signals  ----

  signal present_state, next_state : state := idle;


  signal Header_ES, Footer_ES : unsigned (31 downto 0);
  signal frameCounter : unsigned (31 downto 0);
  signal startOfFrame : std_logic := '0';
  --Counters
  signal internalCounter : unsigned (7 downto 0);
  signal internalPeriodicCounter : unsigned (15 downto 0);
  --Register that holds the length of the sequence
  signal sequenceLength : unsigned (7 downto 0) := (others=>'0');
  --Counter Reset
  signal internalReset : std_logic;
  signal counterReset, counterReset2 : std_logic;
  --randomBuses carry the random numbers created by the random generator
  signal randomBus1 : unsigned (15 downto 0) := x"0000";

  -- contatore che si aggiorna ogni volta che si invia un segnale con trigger
  signal TriggerCounter : unsigned (15 downto 0)  := ( others =>'0');

  --latched_trigger
  signal latchedTrigger : std_logic:='0';








begin


  Header_ES <= headerWord(31 downto 16) & sensorTag(15 downto 0);
  Footer_ES <= footerWord(31 downto 16) & sensorTag(15 downto 0);


  --Random Generators Instantiation
  randomGen1 : Random_generator
    port map (
      Clock => Clock,
      Reset => Reset,
      Enable => '1',
      Random_Data => randomBus1
      );

  --Counter instantiation 1:used to count clocks from the beginning and the end of a frame
  --Used for header and footer (constant part)
  counterMachine : Counter_nbit
    generic map( nbit => 8)
    port map (
      Clock => Clock,
      Reset => counterReset,
      Count => internalCounter
      );

  internalReset <= '1' when next_state = idle
                   else '0';
  counterReset <= internalReset or Reset;

  -- Frame counter :  used to count clock from one frame to the next one
  --Used for header and footer (constant part)
  counterMachine2 : Counter_nbit
    generic map( nbit => 16)
    port map (
      Clock => Clock,
      Reset => counterReset2,
      Count => internalPeriodicCounter
      );




  process(Clock, Reset)
  begin
    if Reset = '1' then
      counterReset2 <= '1';
      frameCounter <= x"ffff_ffff";
      startOfFrame <= '0';
      latchedTrigger <='0';
      TriggerCounter <= ( others =>'0');
    elsif rising_edge(Clock) then
      counterReset2 <= '0';
      startOfFrame <= '0';
      if( internalPeriodicCounter= to_unsigned(periodicity, 16) ) then   -- after n° periodicity of clocks, it starts the stream of the frame
        frameCounter <= frameCounter+1;
        startOfFrame <= '1';
        counterReset2 <= '1';
      end if;

      if Trigger = '1' then
        latchedTrigger <= '1';
      elsif (present_state = sequence and latchedTrigger = '1' and internalCounter = 4) then
        latchedTrigger <= '0';
      end if;


      if (latchedTrigger = '1' and internalCounter = "100" and present_state = sequence) then
        TriggerCounter <= TriggerCounter +1;
      end if;

    end if;
  end process;


-- Process number 1
-- Decides the next state of the machine

  process(
    startOfFrame, present_state,
    internalCounter,SequenceLength)
  begin
    case present_state is

      when idle =>
        if startOfFrame = '1' then
          next_state <= lengthDeclaration;
        else
          next_state <= idle;
        end if;

      when lengthDeclaration =>
        next_state <= header;

      when header =>
        next_state <= frame;

      when frame =>
        next_state <= sequence;

      when sequence =>
        if internalCounter >= sequenceLength-to_unsigned(1,8) then                      -- la lunghezza del frame parte dalla prima parola in lengthDeclaration: lunghezza-1 perchè l'ultima parola è il footer
          next_state <= footer;
        else
          next_state<=sequence;
        end if;

      when footer =>
        next_state <= idle;

    end case;
  end process;

  --Process number 2
  --It calculates the Data_Stream output value based on actual internal state and counters

  process( present_state, internalCounter, randomBus1, sequenceLength)
  begin
    case present_state is

      when idle  =>
        Data_Stream <=to_unsigned(0,32);

        --When lengthDeclaration, output is the value that indicates the total length of the sequence
        --The length is a random number created by random_generator1
      when lengthDeclaration =>
        Data_Stream <=to_unsigned(0,24) & sequenceLength;

      when header =>
        Data_Stream <= Header_ES;

      when frame =>
        Data_Stream <= x"F0" & frameCounter(23 downto 0);

      when sequence =>
         if (latchedTrigger= '1' and internalCounter = 4) then
           Data_Stream <= internalCounter & x"11" & (TriggerCounter+1);               -- se è il segnale di trigger, la prima parola inviata è introdotta da internal counter + x'11' + counter dei trigger misurati

         else
           Data_Stream <= internalCounter & x"00" & randomBus1;
           --Triggerprova <= '0';
         end if;

      when footer =>
        Data_Stream <= Footer_ES;

      when others => Data_Stream<=to_unsigned(0,32);

    end case;
  end process;

  --Data_Valid OutPut
  Data_Valid <= '0' when present_state = idle
                else '1';

  --End_Of_Sequence
  EndOfEvent <= '1' when present_state = footer
                else '0';




  --Process number 3
  --Sequential part

  process(Clock, Reset)
  begin
    if Reset = '1' then
      present_state<=idle;
      sequenceLength <= (others=>'0');
    elsif rising_edge(Clock) then
      present_state <= next_state;
      if present_state = idle then
        if randomBus1(5 downto 0) >= to_unsigned (13,8) then
          sequenceLength<="00"&randomBus1(5 downto 0);
        else
          sequenceLength(7 downto 0)<="001" & randomBus1(4 downto 0);
        end if;
      end if;
    end if;
  end process;

end architecture;
