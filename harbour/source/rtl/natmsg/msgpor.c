/*
 * $Id$
 */

#include <hbdefs.h>

char *hb_monthsname[ 12 ] = {
   "Janeiro", "Fevereiro", "Mar�o",
   "Abril", "Maio", "Junho", "Julho",
   "Agosto", "Setembro", "Outubro",
   "Novembro", "Dezembro" };

char *hb_daysname[ 7 ] = {
   "Domingo", "Segunda-feira", "Ter�a-feira",
   "Quarta-feira", "Quinta-feira", "Sexta-feira",
   "S�bado" };

static char *genericErrors[] =
{
   "Erro desconhecido",
   "Erro nos par�metros",
   "Erro de limite",
   "Overflow de string",
   "Overflow num�rico",
   "Divizao por zero",
   "Erro num�rico",
   "Erro de sintaxe",
   "Opera��o muito complexa",
   "",
   "",
   "Mem�ria insuficiente",
   "Fun��o indefinida",
   "M�todo n�o exportado",
   "Vari�vel n�o existe",
   "Alias n�o existe",
   "Nenhuma vari�vel exportada",
   "Nome de alias incorreto",
   "Nome de alias duplicado",
   "",
   "Erro de cria��o",
   "Erro de abertura",
   "Erro ao fechar",
   "Erro de leitura",
   "Erro de escrita",
   "Erro de impress�o",
   "",
   "",
   "",
   "",
   "Opera��o n�o suportada",
   "Limite excedido",
   "Detectado �ndice corrompido",
   "Tipo incorreto de dado",
   "Tamanho do dato muito longo",
   "Workarea n�o est� em uso",
   "Workarea n�o indexada",
   "Uso exclusivo requerido",
   "Travamento requerido",
   "Escrita n�o permitida",
   "Falha no travamento do Append",
   "Falha no travamento",
   "",
   "",
   "",
   "Numero de par�metros incorretos",
   "acesso de array",
   "array assign",
   "n�o � um array",
   "condicional"
};

char *hb_ErrorNatDescription( ULONG ulGenError )
{
   if( ulGenError <= sizeof(genericErrors)/sizeof(char*) )
      return genericErrors[ ulGenError ];
   else
      return genericErrors[ 0 ];
}
