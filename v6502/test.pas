unit test;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, u6502, ExtCtrls;

type
  TForm1 = class(TForm)
    Label1: TLabel;
    Button1: TButton;
    Step: TButton;
    procedure Button1Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure StepClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    v6502:PVirtual6502;
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

procedure TForm1.Button1Click(Sender: TObject);
begin
  close;
end;

procedure DoNothing;
begin
end;

procedure TForm1.FormCreate(Sender: TObject);
var f:file;
    k,l:integer;
begin
  l:=sizeof(v6502^);
  if (l>0) then begin
    v6502:=NIL;
  end;
  v6502:=New6502;
  system.assign(f,'c:\6502\my\delphi\Apple2.rom'); system.reset(f,1);
  l:=FileSize(f);
  system.BlockRead(f,v6502.AddressSpace[65536-l],l,k);
  system.close(f);
  inc(v6502.RomStart,65536-l);
  inc(v6502.SpecialStart,$C000);
  inc(v6502.SpecialEnd,$C100);
  v6502.SpecialWrite:=@DoNothing;
  v6502.SpecialRead:=@DoNothing;
  v6502.PC:=Byte(v6502.AddressSpace[$FFFC])+
            256*Byte(v6502.AddressSpace[$FFFD]);
end;

procedure TForm1.StepClick(Sender: TObject);
begin
  if (v6502<>NIL) then begin
    Execute6502(v6502,1);
    Label1.Caption:=format('$%04X',[v6502.PC]);
  end;
end;

end.
