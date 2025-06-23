
#include "StringExpression.h"

FStringExpression::FStringExpression(const FStringExpression& other) : expressions(other.expressions)
{
}

FStringExpression& FStringExpression::operator=(const FStringExpression& other)
{
	expressions = other.expressions;
	return *this;
}

void FStringExpression::SetExpressionValue(const FString& key, const FString& value)
{
	for (auto& it : expressions)
	{
		if (it.Key == key)
		{
			it.Value = value;
			return;
		}
	}
	expressions.Add({ key.ToLowerCase(), value });
}

FString FStringExpression::GetExpression(const FString& _k)
{
	FString key = _k.ToLowerCase();
	for (auto& k : expressions)
		if (k.Key == key)
			return k.Value;
	return FString();
}

FString FStringExpression::ParseString(const FString& in)
{
	FString curExpression;
	int mode = 0;

	FString out;

	for (int i = 0; i < in.Size(); i++)
	{
		if (mode == 0)
		{
			if (in[i] == '$' && in[i + 1] == '{')
			{
				curExpression.Clear();
				mode = 1;
				i++;
				continue;
			}

			out += in[i];
		}
		else if (mode == 1)
		{
			if (in[i] == '}')
			{
				out += GetExpression(curExpression);
				mode = 0;
				continue;
			}

			curExpression += in[i];
		}
	}

	return out;
}
