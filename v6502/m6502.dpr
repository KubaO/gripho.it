program m6502;

uses
  Forms,
  test in 'test.pas' {Form1},
  u6502 in 'u6502.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
