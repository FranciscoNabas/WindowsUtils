using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using WindowsUtils.TerminalServices;

#nullable enable
namespace WindowsUtils.Abstraction
{
    public abstract class Enumeration : IComparable
    {
        internal string Name { get; set; }

        internal uint Id { get; set; }

        protected Enumeration(uint id, string name) => (Id, Name) = (id, name);

        public override string ToString() => Name;

        internal static IEnumerable<T> GetAll<T>() where T : Enumeration =>
            typeof(T).GetFields(BindingFlags.Public |
                                BindingFlags.Static |
                                BindingFlags.DeclaredOnly)
                     .Select(f => f.GetValue(null))
                     .Cast<T>();

        public override bool Equals(object? obj)
        {
            if (obj is not Enumeration otherValue)
            {
                return false;
            }

            var typeMatches = GetType().Equals(obj.GetType());
            var valueMatches = Id.Equals(otherValue.Id);

            return typeMatches && valueMatches;
        }

        public override int GetHashCode()
        {
            return (Name, Id).GetHashCode();
        }

        public int CompareTo(object? other)
        {
            if (other == null) { return 0; }
            else { return Id.CompareTo(((Enumeration)other).Id); }
        }
    }
    public abstract class MessageBoxOptions : Enumeration
    {
        internal uint Value { get; set; }
        internal MessageBoxOptions(uint value, string name)
            : base(value, name)
        {
            Value = value;
        }

        private static Dictionary<string, string> GetAllNames()
        {
            Dictionary<string, string> allName = new Dictionary<string, string>();
            GetAll<MessageBoxButton>().Select(x => x.Name).ToList().ForEach(s => allName.Add("MessageBoxButton", s));
            GetAll<MessageBoxIcon>().Select(x => x.Name).ToList().ForEach(s => allName.Add("MessageBoxIcon", s));
            GetAll<MessageBoxDefaultButton>().Select(x => x.Name).ToList().ForEach(s => allName.Add("MessageBoxDefaultButton", s));
            GetAll<MessageBoxModal>().Select(x => x.Name).ToList().ForEach(s => allName.Add("MessageBoxModal", s));
            GetAll<MessageBoxType>().Select(x => x.Name).ToList().ForEach(s => allName.Add("MessageBoxType", s));

            return allName;
        }

        internal static List<MessageBoxOptions> MbOptionsResolver(string[] input)
        {
            List<MessageBoxOptions> output = new List<MessageBoxOptions>();
            List<string> processed = new List<string>();
            Dictionary<string, string> allName = GetAllNames();

            foreach (string item in input)
            {
                if (allName.Values.Contains(item))
                {
                    if (processed.Contains(item)) { Utilities.WriteWarning("Duplicate item " + item + ". Ignoring."); }
                    else
                    {
                        string current = allName.Where(s => s.Value == item).FirstOrDefault().Key;
                        switch (current)
                        {
                            case "MessageBoxButton":
                                output.Add(GetAll<MessageBoxButton>().Where(s => s.Name == item).FirstOrDefault());
                                break;
                            case "MessageBoxIcon":
                                output.Add(GetAll<MessageBoxIcon>().Where(s => s.Name == item).FirstOrDefault());
                                break;
                            case "MessageBoxDefaultButton":
                                output.Add(GetAll<MessageBoxDefaultButton>().Where(s => s.Name == item).FirstOrDefault());
                                break;
                            case "MessageBoxModal":
                                output.Add(GetAll<MessageBoxModal>().Where(s => s.Name == item).FirstOrDefault());
                                break;
                            case "MessageBoxType":
                                output.Add(GetAll<MessageBoxType>().Where(s => s.Name == item).FirstOrDefault());
                                break;
                        }
                        processed.Add(item);
                    }
                }
                else { throw new ArgumentOutOfRangeException("invalid type '" + item + "'."); }
            }
            return output;
        }
    }
}
