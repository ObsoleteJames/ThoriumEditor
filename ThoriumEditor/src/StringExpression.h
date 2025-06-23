#pragma once

#include <Util/Core.h>

class FStringExpression
{
public:
	FStringExpression() {}
	FStringExpression(const FStringExpression&);
	FStringExpression& operator=(const FStringExpression& other);

	void SetExpressionValue(const FString& key, const FString& value);
	FString GetExpression(const FString& key);

	FString ParseString(const FString& in);

private:
	TArray<TPair<FString, FString>> expressions;
};
