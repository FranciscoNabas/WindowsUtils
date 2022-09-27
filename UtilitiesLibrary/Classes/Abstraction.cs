using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
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

        public static T GetById<T>(uint id) where T : Enumeration =>
            GetAll<T>().First(f => f.Id == id);
            
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
    public abstract class MessageBoxOption : Enumeration
    {
        public new string Name { get; set; }
        public uint Value { get; set; }
        public Type Type { get; set; }
        public MessageBoxOption(uint value, string name, Type type)
            : base(value, name)
        {
            Name = name;
            Value = value;
            Type = type;
        }

        public static MessageBoxOption[] GetAvailableOptions()
        {
            MessageBoxOption[] output = new MessageBoxOption[23];
            int arrindex = 0;
            GetAll<MessageBoxButton>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxDefaultButton>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxIcon>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxModal>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxType>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));

            return output;
        }

        internal static uint MbOptionsResolver(string[] input)
        {
            List<string> processed = new List<string>();
            MessageBoxOption[] allNames = GetAvailableOptions();
            uint output = 0;

            foreach (string item in input)
            {
                MessageBoxOption current = allNames.Where(p => p.Name == item).FirstOrDefault();

                if (current is not null)
                {
                    if (processed.Contains(item)) { Utilities.WriteWarning("Duplicate item " + item + ". Ignoring."); }
                    else
                    {
                        output = output | current.Value;
                        processed.Add(item);
                    }
                }
                else { throw new ArgumentOutOfRangeException("invalid type '" + item + "'."); }
            }

            return output;
        }
    }
}