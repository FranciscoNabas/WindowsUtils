using System.Text;

namespace WindowsUtils.Installer
{
    internal static class InstallerSqlParser
    {
        private static readonly char[] _wordSeparators = new char[] { ' ', '\n', ',', '(', ')' };

        internal static InstallerCommand Parse(string command)
        {
            SqlActionType type = ReadFirstWord(command) switch {
                "SELECT" => SqlActionType.Select,
                "DELETE" => SqlActionType.Delete,
                "UPDATE" => SqlActionType.Update,
                "INSERT" => SqlActionType.Insert,
                "CREATE" => SqlActionType.Create,
                "DROP" => SqlActionType.Drop,
                "ALTER" => SqlActionType.Alter,
                _ => throw new InvalidCommandException(command)
            };

            return new InstallerCommand(command, type);
        }

        private static string ReadFirstWord(string command)
        {
            StringBuilder sb = new();
            int charIndex = 0;

            bool firstCharFound = false;
            while (charIndex < command.Length) {
                if (_wordSeparators.Contains(command[charIndex])) {
                    if (!firstCharFound) {
                        charIndex++;
                        continue;
                    }
                    else
                        break;
                }
                else {
                    firstCharFound = true;
                    sb.Append(char.ToUpperInvariant(command[charIndex]));
                    charIndex++;
                }
            }

            return sb.ToString();
        }
    }
}