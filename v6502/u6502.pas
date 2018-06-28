unit u6502;

interface

type
  PVirtual6502=^Virtual6502;
  Virtual6502=
    record
      AddressSpace:PChar;
      SpecialStart:PChar;
      SpecialEnd:PChar;
      ROMStart:PChar;
      ticks:integer;
      SpecialWrite:Procedure;
      SpecialRead:Procedure;
      SpecialEA:integer;
      SpecialValue:integer;
      PC:integer;
      A:integer;
      X:integer;
      Y:integer;
      S:integer;
      P:integer;
    end;

function New6502:PVirtual6502; stdcall;
procedure Free6502(v6502:PVirtual6502); stdcall;
function Execute6502(v6502:PVirtual6502;
                     ticks:integer):integer; stdcall;

implementation

function New6502:PVirtual6502; stdcall;
  external 'v6502';
procedure Free6502(v6502:PVirtual6502); stdcall;
  external 'v6502';
function Execute6502(v6502:PVirtual6502;
                     ticks:integer):integer; stdcall;
  external 'v6502';

end.
